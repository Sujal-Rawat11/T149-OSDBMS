// ======================================================
//   ELECTRICITY LOAD MANAGEMENT SYSTEM (Main Entry Point)
// ======================================================
#include <iostream>
#include "db_connection.h"
#include "helpers.h"
#include "consumer.h"
#include "appliance.h"
#include "load_dashboard.h"
#include "load_log.h"
#include "bill.h"
#include "demand_record.h"
#include "supply_source.h"
#include "admin.h"

using namespace std;

void printMainMenu() {
    cout << "\n========================================\n";
    cout << "     ELECTRICITY LOAD MANAGEMENT\n";
    cout << "========================================\n";
    cout << "1. Consumer Management\n";
    cout << "2. Appliance Management\n";
    cout << "3. Check Current Load\n";
    cout << "4. Load Logs\n";
    cout << "5. Electricity Bills\n";
    cout << "6. Demand Records\n";
    cout << "7. Supply Sources\n";
    cout << "8. Admin Login\n";
    cout << "9. Exit\n";
    cout << "----------------------------------------\n";
}

int main() {
    cout << "========================================\n";
    cout << "     ELECTRICITY LOAD MANAGEMENT\n";
    cout << "========================================\n\n";

    // ---- Connect to MySQL & Initialize Tables ----
    MYSQL* db = connectDB();

    int option;
    do {
        printMainMenu();
        option = (int)readNumber("Enter choice (1-9): ", 1, 9);

        switch (option) {
            case 1:
                consumerMenu(db);
                break;
            case 2:
                applianceMenu(db);
                break;
            case 3:
                checkCurrentLoadMenu(db);
                break;
            case 4:
                loadLogMenu(db);
                break;
            case 5:
                billMenu(db);
                break;
            case 6:
                demandMenu(db);
                break;
            case 7:
                supplySourceMenu(db);
                break;
            case 8:
                adminMenu(db);
                break;
            case 9:
                cout << "\nExiting System...\n";
                break;
            default:
                cout << "Invalid Option.\n";
        }
    } while (option != 9);

    mysql_close(db);
    cout << "Database connection closed.\n";
    cout << "Goodbye!\n";
    return 0;
}
