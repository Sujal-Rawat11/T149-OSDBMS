#include "admin.h"
#include "helpers.h"
#include "consumer.h"
#include "appliance.h"
#include "load_log.h"
#include "bill.h"
#include "demand_record.h"
#include "supply_source.h"
#include <iostream>
#include <cstring>

using namespace std;

bool adminLogin(MYSQL* conn) {
    cout << "\n========================================\n";
    cout << "              ADMIN LOGIN               \n";
    cout << "========================================\n";
    string username = readLine("Username: ");
    string password = readLine("Password: ");

    const char* query = "SELECT admin_id FROM admin WHERE username=? AND password=?";
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return false;

    bool success = false;
    if (!mysql_stmt_prepare(stmt, query, strlen(query))) {
        MYSQL_BIND bind[2];
        memset(bind, 0, sizeof(bind));

        bind[0].buffer_type = MYSQL_TYPE_STRING; bind[0].buffer = (void*)username.c_str(); bind[0].buffer_length = username.length();
        bind[1].buffer_type = MYSQL_TYPE_STRING; bind[1].buffer = (void*)password.c_str(); bind[1].buffer_length = password.length();

        if (!mysql_stmt_bind_param(stmt, bind) && !mysql_stmt_execute(stmt)) {
            mysql_stmt_store_result(stmt);
            if (mysql_stmt_num_rows(stmt) > 0) {
                success = true;
            }
        }
    }
    mysql_stmt_close(stmt);

    if (success) {
        cout << "\nLogin successful! Welcome Admin.\n";
    } else {
        cout << "\nInvalid Username or Password.\n";
    }
    return success;
}

void adminMenu(MYSQL* conn) {
    if (!adminLogin(conn)) return;

    int choice;
    do {
        cout << "\n========================================\n";
        cout << "           ADMIN CONTROL PANEL          \n";
        cout << "========================================\n";
        cout << "1. Manage Consumers\n";
        cout << "2. Manage Appliances\n";
        cout << "3. Manage Electricity Bills\n";
        cout << "4. Manage Supply Sources\n";
        cout << "5. View System Load Logs\n";
        cout << "6. View Demand Records & Peak Demand\n";
        cout << "7. Logout Admin\n";
        choice = (int)readNumber("Enter Choice (1-7): ", 1, 7);

        if (choice == 1) consumerMenu(conn);
        else if (choice == 2) applianceMenu(conn);
        else if (choice == 3) billMenu(conn);
        else if (choice == 4) supplySourceMenu(conn);
        else if (choice == 5) loadLogMenu(conn);
        else if (choice == 6) demandMenu(conn);
    } while (choice != 7);

    cout << "Logged out of Admin Control Panel.\n";
}
