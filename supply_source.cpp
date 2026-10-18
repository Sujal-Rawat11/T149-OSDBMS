#include "supply_source.h"
#include "helpers.h"
#include <iostream>
#include <iomanip>
#include <cstring>

using namespace std;

void addSupplySource(MYSQL* conn) {
    cout << "\n--- ADD NEW SUPPLY SOURCE ---\n";
    string name = readLine("Source Name (e.g. Solar Array 1): ");
    
    cout << "Select Source Type:\n1. Grid\n2. Solar\n3. Generator\n4. Battery\n5. Other\n";
    int tChoice = (int)readNumber("Choice (1-5): ", 1, 5);
    const string types[] = {"", "Grid", "Solar", "Generator", "Battery", "Other"};
    string type = types[tChoice];

    float capacity = readNumber("Capacity (Watts): ", 1);
    float available = readNumber("Available Output (Watts): ", 0, capacity);

    const char* query = "INSERT INTO supply_sources (source_name, source_type, capacity_watts, available_watts, status) VALUES (?, ?, ?, ?, 'ACTIVE')";
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return;

    if (!mysql_stmt_prepare(stmt, query, strlen(query))) {
        MYSQL_BIND bind[4];
        memset(bind, 0, sizeof(bind));

        bind[0].buffer_type = MYSQL_TYPE_STRING; bind[0].buffer = (void*)name.c_str(); bind[0].buffer_length = name.length();
        bind[1].buffer_type = MYSQL_TYPE_STRING; bind[1].buffer = (void*)type.c_str(); bind[1].buffer_length = type.length();
        bind[2].buffer_type = MYSQL_TYPE_FLOAT; bind[2].buffer = &capacity;
        bind[3].buffer_type = MYSQL_TYPE_FLOAT; bind[3].buffer = &available;

        if (!mysql_stmt_bind_param(stmt, bind) && !mysql_stmt_execute(stmt)) {
            int sid = (int)mysql_stmt_insert_id(stmt);
            cout << "Supply Source added successfully! (Source ID: " << sid << ")\n";
        } else {
            cerr << "Failed to add supply source.\n";
        }
    }
    mysql_stmt_close(stmt);
}

void viewSupplySources(MYSQL* conn) {
    cout << "\n========================================================================================\n";
    cout << "                                  ELECTRICITY SUPPLY SOURCES\n";
    cout << "========================================================================================\n";

    const char* query = "SELECT source_id, source_name, source_type, capacity_watts, available_watts, status FROM supply_sources";

    if (mysql_query(conn, query)) {
        cerr << "Query failed: " << mysql_error(conn) << "\n";
        return;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return;

    cout << left << setw(8) << "ID"
         << setw(26) << "Name"
         << setw(14) << "Type"
         << setw(16) << "Capacity(W)"
         << setw(16) << "Available(W)"
         << "Status\n";
    cout << "----------------------------------------------------------------------------------------\n";

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        cout << left << setw(8) << (row[0] ? row[0] : "")
             << setw(26) << (row[1] ? row[1] : "")
             << setw(14) << (row[2] ? row[2] : "")
             << setw(16) << (row[3] ? row[3] : "")
             << setw(16) << (row[4] ? row[4] : "")
             << (row[5] ? row[5] : "") << "\n";
    }
    cout << "----------------------------------------------------------------------------------------\n";
    mysql_free_result(res);
}

void updateSourceCapacity(MYSQL* conn) {
    viewSupplySources(conn);
    int sid = (int)readNumber("Enter Source ID to update: ", 1);
    float capacity = readNumber("New Total Capacity (Watts): ", 1);
    float available = readNumber("New Available Output (Watts): ", 0, capacity);

    const char* query = "UPDATE supply_sources SET capacity_watts=?, available_watts=? WHERE source_id=?";
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return;

    if (!mysql_stmt_prepare(stmt, query, strlen(query))) {
        MYSQL_BIND bind[3];
        memset(bind, 0, sizeof(bind));

        bind[0].buffer_type = MYSQL_TYPE_FLOAT; bind[0].buffer = &capacity;
        bind[1].buffer_type = MYSQL_TYPE_FLOAT; bind[1].buffer = &available;
        bind[2].buffer_type = MYSQL_TYPE_LONG; bind[2].buffer = &sid;

        if (!mysql_stmt_bind_param(stmt, bind) && !mysql_stmt_execute(stmt)) {
            cout << "Supply Source updated successfully.\n";
        } else {
            cerr << "Failed to update supply source.\n";
        }
    }
    mysql_stmt_close(stmt);
}

void toggleSourceStatus(MYSQL* conn) {
    viewSupplySources(conn);
    int sid = (int)readNumber("Enter Source ID to toggle ACTIVE/INACTIVE: ", 1);
    cout << "Select Status:\n1. ACTIVE\n2. INACTIVE\n";
    int sChoice = (int)readNumber("Choice (1-2): ", 1, 2);
    string newStatus = (sChoice == 1) ? "ACTIVE" : "INACTIVE";

    const char* query = "UPDATE supply_sources SET status=? WHERE source_id=?";
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return;

    if (!mysql_stmt_prepare(stmt, query, strlen(query))) {
        MYSQL_BIND bind[2];
        memset(bind, 0, sizeof(bind));

        bind[0].buffer_type = MYSQL_TYPE_STRING; bind[0].buffer = (void*)newStatus.c_str(); bind[0].buffer_length = newStatus.length();
        bind[1].buffer_type = MYSQL_TYPE_LONG; bind[1].buffer = &sid;

        if (!mysql_stmt_bind_param(stmt, bind) && !mysql_stmt_execute(stmt)) {
            cout << "Source status updated to " << newStatus << ".\n";
        } else {
            cerr << "Failed to update status.\n";
        }
    }
    mysql_stmt_close(stmt);
}

float getTotalActiveCapacity(MYSQL* conn) {
    const char* query = "SELECT SUM(available_watts) FROM supply_sources WHERE status='ACTIVE'";
    if (mysql_query(conn, query)) return 0.0f;

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return 0.0f;

    MYSQL_ROW row = mysql_fetch_row(res);
    float total = row[0] ? atof(row[0]) : 0.0f;
    mysql_free_result(res);
    return total;
}

void supplySourceMenu(MYSQL* conn) {
    int choice;
    do {
        cout << "\n--- SUPPLY SOURCES MENU ---\n";
        cout << "1. View All Supply Sources\n";
        cout << "2. Add New Supply Source\n";
        cout << "3. Update Source Capacity\n";
        cout << "4. Activate / Deactivate Source\n";
        cout << "5. Back to Main Menu\n";
        choice = (int)readNumber("Enter Choice (1-5): ", 1, 5);

        if (choice == 1) viewSupplySources(conn);
        else if (choice == 2) addSupplySource(conn);
        else if (choice == 3) updateSourceCapacity(conn);
        else if (choice == 4) toggleSourceStatus(conn);
    } while (choice != 5);
}
