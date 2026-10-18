// ======================================================
//   ELECTRICITY LOAD MANAGEMENT  (single file version)
//   with MySQL Database Integration
// ======================================================
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <cstdlib>
#include <winsock2.h>       // must come before mysql.h on Windows
#include <mysql.h>
using namespace std;

// ---------- database credentials ----------
const char* DB_HOST = "localhost";
const char* DB_USER = "root";
const char* DB_PASS = "sujal8057";
const char* DB_NAME = "electricity_load_management";
const unsigned int DB_PORT = 3306;

// ---------- data ----------
struct Appliance {
    int dbId;       // database primary key (-1 if not yet saved)
    string name;
    float power;    // Watts
    int priority;   // 1 = most important ... 5 = least important
    bool on;        // decided by the load manager
};


// ==================== DATABASE HELPERS ====================

MYSQL* connectDB() {
    MYSQL* conn = mysql_init(NULL);
    if (!conn) {
        cerr << "mysql_init() failed.\n";
        exit(1);
    }
    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS,
                            DB_NAME, DB_PORT, NULL, 0)) {
        cerr << "Database connection failed: " << mysql_error(conn) << "\n";
        mysql_close(conn);
        exit(1);
    }
    cout << "Connected to MySQL database successfully.\n";
    return conn;
}

// Insert a new consumer row; returns the auto-generated ID.
int dbSaveConsumer(MYSQL* conn, const string& type, float capacity) {
    char query[512];
    sprintf(query,
        "INSERT INTO consumers (consumer_type, capacity_watts) VALUES ('%s', %.2f)",
        type.c_str(), capacity);
    if (mysql_query(conn, query)) {
        cerr << "DB Error (saveConsumer): " << mysql_error(conn) << "\n";
        return -1;
    }
    return (int)mysql_insert_id(conn);
}

// Insert a new appliance row; sets a.dbId to the generated ID.
void dbSaveAppliance(MYSQL* conn, int consumerId, Appliance& a) {
    char query[512];
    sprintf(query,
        "INSERT INTO appliances (consumer_id, name, power_watts, priority, is_on) "
        "VALUES (%d, '%s', %.2f, %d, %d)",
        consumerId, a.name.c_str(), a.power, a.priority, a.on ? 1 : 0);
    if (mysql_query(conn, query)) {
        cerr << "DB Error (saveAppliance): " << mysql_error(conn) << "\n";
        return;
    }
    a.dbId = (int)mysql_insert_id(conn);
}

// Delete an appliance row by its database ID.
void dbDeleteAppliance(MYSQL* conn, int dbId) {
    if (dbId < 0) return;
    char query[256];
    sprintf(query, "DELETE FROM appliances WHERE id = %d", dbId);
    if (mysql_query(conn, query))
        cerr << "DB Error (deleteAppliance): " << mysql_error(conn) << "\n";
}

// Update the capacity of a consumer.
void dbUpdateCapacity(MYSQL* conn, int consumerId, float capacity) {
    char query[256];
    sprintf(query,
        "UPDATE consumers SET capacity_watts = %.2f WHERE id = %d",
        capacity, consumerId);
    if (mysql_query(conn, query))
        cerr << "DB Error (updateCapacity): " << mysql_error(conn) << "\n";
}

// Update priority of a single appliance.
void dbUpdatePriority(MYSQL* conn, int dbId, int priority) {
    if (dbId < 0) return;
    char query[256];
    sprintf(query,
        "UPDATE appliances SET priority = %d WHERE id = %d",
        priority, dbId);
    if (mysql_query(conn, query))
        cerr << "DB Error (updatePriority): " << mysql_error(conn) << "\n";
}

// After balanceLoad(), sync ON/OFF status of all appliances to the DB.
void dbSyncStatuses(MYSQL* conn, const vector<Appliance>& list) {
    for (const auto& a : list) {
        if (a.dbId < 0) continue;
        char query[256];
        sprintf(query,
            "UPDATE appliances SET is_on = %d WHERE id = %d",
            a.on ? 1 : 0, a.dbId);
        if (mysql_query(conn, query))
            cerr << "DB Error (syncStatus): " << mysql_error(conn) << "\n";
    }
}


// ==================== ORIGINAL LOGIC (preserved) ====================

// ---------- input helpers + priority ----------
string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

string readLine(const string& prompt) {
    string s;
    while (true) {
        cout << prompt;
        if (!getline(cin, s)) exit(0);          // input ended
        if (!s.empty()) return s;
        cout << "Cannot be empty.\n";
    }
}

float readNumber(const string& prompt, float min, float max = 1e9) {
    float x;
    while (true) {
        cout << prompt;
        if (cin >> x && x >= min && x <= max) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return x;
        }
        if (cin.eof()) exit(0);                 // input ended
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input, try again.\n";
    }
}

// ---------- priority rules ----------
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
    return 2; // Other
}

int askPriority(const string& type, const string& name) {
    int suggested = autoPriority(type, name);
    cout << "Suggested priority: " << suggested << "  (1 = most important, 5 = least)\n";
    int manual = (int)readNumber("Enter priority 1-5 (0 = accept suggestion): ", 0, 5);
    return manual == 0 ? suggested : manual;
}

// ---------- load manager ----------
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

// Shed: while overloaded, switch OFF the least important appliance
// (highest priority number; on a tie, the one using more power).
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

// Restore: go through OFF appliances, most important first, and switch each
// ON if it fits. Stops at the first one that doesn't fit, so a less important
// appliance never runs while a more important one is still OFF.
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

// Recalculate everything, then report which appliances changed state.
void balanceLoad(vector<Appliance>& list, float capacity) {
    vector<bool> before;
    for (const auto& a : list) before.push_back(a.on);

    for (auto& a : list) a.on = true;   // start from full demand
    shedLoad(list, capacity);
    restoreLoad(list, capacity);

    cout << "\n";
    for (int i = 0; i < (int)list.size(); i++) {
        if (before[i] && !list[i].on)  cout << ">> SHED: "     << list[i].name << "\n";
        if (!before[i] && list[i].on)  cout << ">> RESTORED: " << list[i].name << "\n";
    }
}

string warningLevel(float used, float capacity) {
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

// ---------- menu + report ----------
void printMenu() {
    cout << "\n---------------- MENU ----------------\n";
    cout << "1. Add appliance\n";
    cout << "2. Remove appliance\n";
    cout << "3. Change available capacity\n";
    cout << "4. Change appliance priority\n";
    cout << "5. Show report\n";
    cout << "6. Exit\n";
    cout << "--------------------------------------\n";
}

void printReport(const string& type, const vector<Appliance>& list, float capacity) {
    float demand = demandOf(list);
    float used = usedPower(list);

    cout << "\n========================================\n";
    cout << "              LOAD REPORT\n";
    cout << "========================================\n";
    cout << "Consumer Type : " << type << "\n";
    cout << "Total Demand  : " << demand << " W\n";
    cout << "Capacity      : " << capacity << " W\n\n";

    if (list.empty()) {
        cout << "No appliances added yet.\n";
        return;
    }

    cout << left << setw(4) << "No."
         << setw(20) << "Appliance"
         << setw(10) << "Power(W)"
         << setw(10) << "Priority"
         << "Status\n";
    cout << "-------------------------------------------------------\n";

    for (int i = 0; i < (int)list.size(); i++) {
        cout << left << setw(4) << i + 1
             << setw(20) << list[i].name
             << setw(10) << list[i].power
             << setw(10) << list[i].priority
             << (list[i].on ? "ON" : "OFF (shed)") << "\n";
    }

    cout << "-------------------------------------------------------\n";
    cout << "Load in use   : " << used << " W (";
    cout << fixed << setprecision(1) << used / capacity * 100 << "%)";
    cout << defaultfloat << setprecision(6);
    cout << "\nLevel         : " << warningLevel(used, capacity) << "\n";

    if (demand > capacity)
        cout << "Note          : Demand exceeds capacity, low priority loads are OFF.\n";
    if (essentialShed(list))
        cout << "ALERT         : Priority 1 (essential) load is OFF! Increase capacity.\n";
}

// ---------- main ----------
int main() {
    cout << "========================================\n";
    cout << "     ELECTRICITY LOAD MANAGEMENT\n";
    cout << "========================================\n\n";

    // ---- Connect to MySQL ----
    MYSQL* db = connectDB();

    cout << "1. Home\n2. Industry\n3. Hospital\n4. Other\n";
    int choice = (int)readNumber("Select consumer type (1-4): ", 1, 4);
    const string types[] = {"", "Home", "Industry", "Hospital", "Other"};
    string userType = types[choice];

    float capacity = readNumber("Available power capacity (Watts): ", 1);
    vector<Appliance> list;

    // ---- Save consumer to DB ----
    int consumerId = dbSaveConsumer(db, userType, capacity);
    if (consumerId < 0) {
        cerr << "Failed to create consumer session in database.\n";
        mysql_close(db);
        return 1;
    }
    cout << "Session saved to database (Consumer ID: " << consumerId << ")\n";

    int option;
    do {
        printMenu();
        option = (int)readNumber("Enter choice: ", 1, 6);

        if (option == 1) {                                   // add
            Appliance a;
            a.dbId = -1;
            a.name = readLine("Name: ");
            a.power = readNumber("Power (Watts): ", 1);
            a.priority = askPriority(userType, a.name);
            a.on = true;
            list.push_back(a);
            balanceLoad(list, capacity);
            // ---- DB: save new appliance, then sync all statuses ----
            dbSaveAppliance(db, consumerId, list.back());
            dbSyncStatuses(db, list);
            printReport(userType, list, capacity);
        }
        else if (option == 2) {                              // remove
            if (list.empty()) { cout << "Nothing to remove.\n"; continue; }
            printReport(userType, list, capacity);
            int n = (int)readNumber("Appliance number to remove: ", 1, (float)list.size());
            // ---- DB: delete from database ----
            dbDeleteAppliance(db, list[n - 1].dbId);
            list.erase(list.begin() + (n - 1));
            balanceLoad(list, capacity);                     // freed power may restore others
            dbSyncStatuses(db, list);
            printReport(userType, list, capacity);
        }
        else if (option == 3) {                              // change capacity
            capacity = readNumber("New available capacity (Watts): ", 1);
            // ---- DB: update capacity ----
            dbUpdateCapacity(db, consumerId, capacity);
            balanceLoad(list, capacity);
            dbSyncStatuses(db, list);
            printReport(userType, list, capacity);
        }
        else if (option == 4) {                              // manual priority
            if (list.empty()) { cout << "No appliances yet.\n"; continue; }
            printReport(userType, list, capacity);
            int n = (int)readNumber("Appliance number: ", 1, (float)list.size());
            list[n - 1].priority = (int)readNumber("New priority (1-5): ", 1, 5);
            // ---- DB: update priority ----
            dbUpdatePriority(db, list[n - 1].dbId, list[n - 1].priority);
            balanceLoad(list, capacity);
            dbSyncStatuses(db, list);
            printReport(userType, list, capacity);
        }
        else if (option == 5) {                              // report
            printReport(userType, list, capacity);
        }
    } while (option != 6);

    // ---- Close MySQL connection ----
    mysql_close(db);
    cout << "\nDatabase connection closed.";
    cout << "\nGoodbye!\n";
    return 0;
}
