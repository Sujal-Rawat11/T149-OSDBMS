#include "appliance.h"
#include "consumer.h"
#include "load_log.h"
#include "helpers.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstring>

using namespace std;

int autoPriority(const string& type, string name) {
    name = toLower(name);
    if (type == "Hospital") {
        if (name == "ventilator" || name == "icu" || name == "oxygen") return 1;
        return 2;
    }
    if (type == "Industry") {
        if (name == "machine" || name == "motor" || name == "pump") return 1;
        return 2;
    }
    if (type == "Home") {
        if (name == "fridge" || name == "fan") return 1;
        if (name == "tv" || name == "computer") return 2;
        return 3;
    }
    return 2;
}

int askPriority(const string& type, const string& name) {
    int suggested = autoPriority(type, name);
    cout << "Suggested priority: " << priorityLabel(suggested) << " (" << suggested << ")\n";
    cout << "Priorities: 1=HIGH, 3=MEDIUM, 5=LOW\n";
    int manual = (int)readNumber("Enter priority 1-5 (0 = accept suggestion): ", 0, 5);
    return manual == 0 ? suggested : manual;
}

float demandOf(const vector<Appliance>& list) {
    float total = 0;
    for (const auto& a : list) total += a.power;
    return total;
}

float usedPower(const vector<Appliance>& list) {
    float total = 0;
    for (const auto& a : list) if (a.on) total += a.power;
    return total;
}

void shedLoad(vector<Appliance>& list, float capacity) {
    while (usedPower(list) > capacity) {
        int worst = -1;
        for (int i = 0; i < (int)list.size(); i++) {
            if (!list[i].on) continue;
            if (worst == -1 ||
                list[i].priority > list[worst].priority ||
                (list[i].priority == list[worst].priority && list[i].power > list[worst].power))
                worst = i;
        }
        if (worst == -1) break;
        list[worst].on = false;
    }
}

void restoreLoad(vector<Appliance>& list, float capacity) {
    vector<int> off;
    for (int i = 0; i < (int)list.size(); i++)
        if (!list[i].on) off.push_back(i);

    stable_sort(off.begin(), off.end(), [&](int a, int b) {
        return list[a].priority < list[b].priority;
    });

    float used = usedPower(list);
    for (int idx : off) {
        if (used + list[idx].power <= capacity) {
            list[idx].on = true;
            used += list[idx].power;
        } else {
            break;
        }
    }
}

void balanceLoad(vector<Appliance>& list, float capacity) {
    vector<bool> before;
    for (const auto& a : list) before.push_back(a.on);

    for (auto& a : list) a.on = true;
    shedLoad(list, capacity);
    restoreLoad(list, capacity);

    cout << "\n";
    for (int i = 0; i < (int)list.size(); i++) {
        if (before[i] && !list[i].on)  cout << ">> SHED: "     << list[i].name << "\n";
        if (!before[i] && list[i].on)  cout << ">> RESTORED: " << list[i].name << "\n";
    }
}

string warningLevel(float used, float capacity) {
    if (capacity <= 0) return "CRITICAL";
    float pct = used / capacity * 100;
    if (pct < 70) return "SAFE";
    if (pct < 90) return "WARNING";
    return "CRITICAL";
}

bool essentialShed(const vector<Appliance>& list) {
    for (const auto& a : list)
        if (a.priority == 1 && !a.on) return true;
    return false;
}

void dbSaveAppliance(MYSQL* conn, int consumerId, Appliance& a) {
    const char* query = "INSERT INTO appliances (consumer_id, name, power_watts, priority, is_on) VALUES (?, ?, ?, ?, ?)";
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return;

    if (!mysql_stmt_prepare(stmt, query, strlen(query))) {
        MYSQL_BIND bind[5];
        memset(bind, 0, sizeof(bind));

        int isOnVal = a.on ? 1 : 0;
        bind[0].buffer_type = MYSQL_TYPE_LONG; bind[0].buffer = &consumerId;
        bind[1].buffer_type = MYSQL_TYPE_STRING; bind[1].buffer = (void*)a.name.c_str(); bind[1].buffer_length = a.name.length();
        bind[2].buffer_type = MYSQL_TYPE_FLOAT; bind[2].buffer = &a.power;
        bind[3].buffer_type = MYSQL_TYPE_LONG; bind[3].buffer = &a.priority;
        bind[4].buffer_type = MYSQL_TYPE_LONG; bind[4].buffer = &isOnVal;

        if (!mysql_stmt_bind_param(stmt, bind) && !mysql_stmt_execute(stmt)) {
            a.dbId = (int)mysql_stmt_insert_id(stmt);
            dbInsertLoadLog(conn, consumerId, a.dbId, a.power, a.on ? "ON" : "OFF");
        }
    }
    mysql_stmt_close(stmt);
}

void dbDeleteAppliance(MYSQL* conn, int dbId) {
    if (dbId < 0) return;
    const char* query = "DELETE FROM appliances WHERE id=?";
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return;

    if (!mysql_stmt_prepare(stmt, query, strlen(query))) {
        MYSQL_BIND bind[1];
        memset(bind, 0, sizeof(bind));
        bind[0].buffer_type = MYSQL_TYPE_LONG; bind[0].buffer = &dbId;
        mysql_stmt_bind_param(stmt, bind);
        mysql_stmt_execute(stmt);
    }
    mysql_stmt_close(stmt);
}

void dbUpdatePriority(MYSQL* conn, int dbId, int priority) {
    if (dbId < 0) return;
    const char* query = "UPDATE appliances SET priority=? WHERE id=?";
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return;

    if (!mysql_stmt_prepare(stmt, query, strlen(query))) {
        MYSQL_BIND bind[2];
        memset(bind, 0, sizeof(bind));
        bind[0].buffer_type = MYSQL_TYPE_LONG; bind[0].buffer = &priority;
        bind[1].buffer_type = MYSQL_TYPE_LONG; bind[1].buffer = &dbId;
        mysql_stmt_bind_param(stmt, bind);
        mysql_stmt_execute(stmt);
    }
    mysql_stmt_close(stmt);
}

void dbUpdateApplianceStatus(MYSQL* conn, int dbId, bool isOn) {
    if (dbId < 0) return;
    const char* query = "UPDATE appliances SET is_on=? WHERE id=?";
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return;

    int isOnVal = isOn ? 1 : 0;
    if (!mysql_stmt_prepare(stmt, query, strlen(query))) {
        MYSQL_BIND bind[2];
        memset(bind, 0, sizeof(bind));
        bind[0].buffer_type = MYSQL_TYPE_LONG; bind[0].buffer = &isOnVal;
        bind[1].buffer_type = MYSQL_TYPE_LONG; bind[1].buffer = &dbId;
        mysql_stmt_bind_param(stmt, bind);
        mysql_stmt_execute(stmt);
    }
    mysql_stmt_close(stmt);
}

void dbSyncStatuses(MYSQL* conn, int consumerId, const vector<Appliance>& list) {
    for (const auto& a : list) {
        if (a.dbId < 0) continue;
        dbUpdateApplianceStatus(conn, a.dbId, a.on);
    }
}

vector<Appliance> loadConsumerAppliances(MYSQL* conn, int consumerId) {
    vector<Appliance> list;
    const char* query = "SELECT id, name, power_watts, priority, is_on FROM appliances WHERE consumer_id=?";
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return list;

    if (!mysql_stmt_prepare(stmt, query, strlen(query))) {
        MYSQL_BIND bind[1];
        memset(bind, 0, sizeof(bind));
        bind[0].buffer_type = MYSQL_TYPE_LONG; bind[0].buffer = &consumerId;

        if (!mysql_stmt_bind_param(stmt, bind) && !mysql_stmt_execute(stmt)) {
            MYSQL_BIND resBind[5];
            memset(resBind, 0, sizeof(resBind));

            int idVal = 0, priVal = 0, isOnVal = 0;
            char nameBuf[101] = {0};
            float powerVal = 0.0f;

            resBind[0].buffer_type = MYSQL_TYPE_LONG; resBind[0].buffer = &idVal;
            resBind[1].buffer_type = MYSQL_TYPE_STRING; resBind[1].buffer = nameBuf; resBind[1].buffer_length = 100;
            resBind[2].buffer_type = MYSQL_TYPE_FLOAT; resBind[2].buffer = &powerVal;
            resBind[3].buffer_type = MYSQL_TYPE_LONG; resBind[3].buffer = &priVal;
            resBind[4].buffer_type = MYSQL_TYPE_LONG; resBind[4].buffer = &isOnVal;

            mysql_stmt_bind_result(stmt, resBind);
            while (!mysql_stmt_fetch(stmt)) {
                Appliance a;
                a.dbId = idVal;
                a.name = nameBuf;
                a.power = powerVal;
                a.priority = priVal;
                a.on = (isOnVal == 1);
                list.push_back(a);
            }
        }
    }
    mysql_stmt_close(stmt);
    return list;
}

void addApplianceInteractive(MYSQL* conn, int consumerId) {
    Consumer c = getConsumerById(conn, consumerId);
    if (c.id == -1) {
        cout << "Invalid Consumer ID.\n";
        return;
    }
    Appliance a;
    a.name = readLine("Appliance Name: ");
    a.power = readNumber("Power Consumption (Watts): ", 1);
    a.priority = askPriority(c.type, a.name);
    a.on = true;

    vector<Appliance> list = loadConsumerAppliances(conn, consumerId);
    list.push_back(a);
    balanceLoad(list, c.capacity);
    dbSaveAppliance(conn, consumerId, list.back());
    dbSyncStatuses(conn, consumerId, list);
    cout << "Appliance added and load re-balanced!\n";
}

void viewAppliances(MYSQL* conn, int consumerId) {
    vector<Appliance> list = loadConsumerAppliances(conn, consumerId);
    cout << "\n--- APPLIANCES FOR CONSUMER ID " << consumerId << " ---\n";
    if (list.empty()) {
        cout << "No appliances registered.\n";
        return;
    }
    cout << left << setw(6) << "DB ID"
         << setw(20) << "Name"
         << setw(12) << "Power(W)"
         << setw(12) << "Priority"
         << "Status\n";
    cout << "---------------------------------------------------------\n";
    for (const auto& a : list) {
        cout << left << setw(6) << a.dbId
             << setw(20) << a.name
             << setw(12) << a.power
             << setw(12) << (priorityLabel(a.priority) + " (" + to_string(a.priority) + ")")
             << (a.on ? "ON" : "OFF (shed)") << "\n";
    }
    cout << "---------------------------------------------------------\n";
}

void turnApplianceOnOff(MYSQL* conn, int consumerId) {
    viewAppliances(conn, consumerId);
    int dbId = (int)readNumber("Enter Appliance DB ID to toggle state: ", 1);
    vector<Appliance> list = loadConsumerAppliances(conn, consumerId);
    int targetIdx = -1;
    for (size_t i = 0; i < list.size(); i++) {
        if (list[i].dbId == dbId) {
            targetIdx = (int)i;
            break;
        }
    }

    if (targetIdx == -1) {
        cout << "Appliance not found.\n";
        return;
    }

    list[targetIdx].on = !list[targetIdx].on;
    dbUpdateApplianceStatus(conn, list[targetIdx].dbId, list[targetIdx].on);
    dbInsertLoadLog(conn, consumerId, list[targetIdx].dbId, list[targetIdx].power, list[targetIdx].on ? "MANUAL_ON" : "MANUAL_OFF");
    cout << "Appliance state updated to: " << (list[targetIdx].on ? "ON" : "OFF") << "\n";
}

void applianceMenu(MYSQL* conn) {
    viewConsumers(conn);
    int consumerId = (int)readNumber("Enter Consumer ID to manage appliances: ", 1);
    if (!consumerExists(conn, consumerId)) {
        cout << "Consumer not found.\n";
        return;
    }

    int choice;
    do {
        cout << "\n--- APPLIANCE MANAGEMENT (Consumer ID: " << consumerId << ") ---\n";
        cout << "1. Add Appliance\n";
        cout << "2. View Appliances\n";
        cout << "3. Toggle Appliance ON/OFF\n";
        cout << "4. Delete Appliance\n";
        cout << "5. Back to Main Menu\n";
        choice = (int)readNumber("Enter Choice (1-5): ", 1, 5);

        if (choice == 1) addApplianceInteractive(conn, consumerId);
        else if (choice == 2) viewAppliances(conn, consumerId);
        else if (choice == 3) turnApplianceOnOff(conn, consumerId);
        else if (choice == 4) {
            viewAppliances(conn, consumerId);
            int dbId = (int)readNumber("Enter Appliance DB ID to delete: ", 1);
            dbDeleteAppliance(conn, dbId);
            cout << "Appliance deleted.\n";
        }
    } while (choice != 5);
}
