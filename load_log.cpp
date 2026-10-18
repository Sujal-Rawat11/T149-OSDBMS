#include "load_log.h"
#include "consumer.h"
#include "helpers.h"
#include <iostream>
#include <iomanip>
#include <cstring>

using namespace std;

void dbInsertLoadLog(MYSQL* conn, int consumerId, int applianceId, float powerWatts, const string& status) {
    const char* query = "INSERT INTO load_logs (consumer_id, appliance_id, power_watts, status) VALUES (?, ?, ?, ?)";
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return;

    if (!mysql_stmt_prepare(stmt, query, strlen(query))) {
        MYSQL_BIND bind[4];
        memset(bind, 0, sizeof(bind));

        bind[0].buffer_type = MYSQL_TYPE_LONG; bind[0].buffer = &consumerId;
        if (applianceId > 0) {
            bind[1].buffer_type = MYSQL_TYPE_LONG; bind[1].buffer = &applianceId;
        } else {
            bind[1].buffer_type = MYSQL_TYPE_NULL;
        }
        bind[2].buffer_type = MYSQL_TYPE_FLOAT; bind[2].buffer = &powerWatts;
        bind[3].buffer_type = MYSQL_TYPE_STRING; bind[3].buffer = (void*)status.c_str(); bind[3].buffer_length = status.length();

        mysql_stmt_bind_param(stmt, bind);
        mysql_stmt_execute(stmt);
    }
    mysql_stmt_close(stmt);
}

void viewLoadLogs(MYSQL* conn) {
    cout << "\n========================================================================================\n";
    cout << "                                    LOAD LOGS HISTORY\n";
    cout << "========================================================================================\n";
    
    const char* query = 
        "SELECT l.log_id, c.name, COALESCE(a.name, 'N/A'), l.power_watts, l.status, l.timestamp "
        "FROM load_logs l "
        "JOIN consumers c ON l.consumer_id = c.id "
        "LEFT JOIN appliances a ON l.appliance_id = a.id "
        "ORDER BY l.timestamp DESC LIMIT 50";

    if (mysql_query(conn, query)) {
        cerr << "Query failed: " << mysql_error(conn) << "\n";
        return;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return;

    cout << left << setw(8) << "Log ID"
         << setw(20) << "Consumer"
         << setw(20) << "Appliance"
         << setw(12) << "Power(W)"
         << setw(12) << "Status"
         << "Timestamp\n";
    cout << "----------------------------------------------------------------------------------------\n";

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        cout << left << setw(8) << (row[0] ? row[0] : "")
             << setw(20) << (row[1] ? row[1] : "")
             << setw(20) << (row[2] ? row[2] : "")
             << setw(12) << (row[3] ? row[3] : "")
             << setw(12) << (row[4] ? row[4] : "")
             << (row[5] ? row[5] : "") << "\n";
    }
    cout << "----------------------------------------------------------------------------------------\n";
    mysql_free_result(res);
}

void viewConsumerLogs(MYSQL* conn, int consumerId) {
    cout << "\n--- LOAD LOGS FOR CONSUMER ID " << consumerId << " ---\n";
    const char* query = 
        "SELECT l.log_id, COALESCE(a.name, 'N/A'), l.power_watts, l.status, l.timestamp "
        "FROM load_logs l "
        "LEFT JOIN appliances a ON l.appliance_id = a.id "
        "WHERE l.consumer_id = ? "
        "ORDER BY l.timestamp DESC";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return;

    if (!mysql_stmt_prepare(stmt, query, strlen(query))) {
        MYSQL_BIND bind[1];
        memset(bind, 0, sizeof(bind));
        bind[0].buffer_type = MYSQL_TYPE_LONG; bind[0].buffer = &consumerId;

        if (!mysql_stmt_bind_param(stmt, bind) && !mysql_stmt_execute(stmt)) {
            MYSQL_BIND resBind[5];
            memset(resBind, 0, sizeof(resBind));

            int logId = 0;
            char appName[101] = {0}, statusBuf[21] = {0}, timeBuf[51] = {0};
            float pwr = 0.0f;

            resBind[0].buffer_type = MYSQL_TYPE_LONG; resBind[0].buffer = &logId;
            resBind[1].buffer_type = MYSQL_TYPE_STRING; resBind[1].buffer = appName; resBind[1].buffer_length = 100;
            resBind[2].buffer_type = MYSQL_TYPE_FLOAT; resBind[2].buffer = &pwr;
            resBind[3].buffer_type = MYSQL_TYPE_STRING; resBind[3].buffer = statusBuf; resBind[3].buffer_length = 20;
            resBind[4].buffer_type = MYSQL_TYPE_STRING; resBind[4].buffer = timeBuf; resBind[4].buffer_length = 50;

            mysql_stmt_bind_result(stmt, resBind);
            cout << left << setw(8) << "Log ID"
                 << setw(20) << "Appliance"
                 << setw(12) << "Power(W)"
                 << setw(12) << "Status"
                 << "Timestamp\n";
            cout << "----------------------------------------------------------------------\n";

            while (!mysql_stmt_fetch(stmt)) {
                cout << left << setw(8) << logId
                     << setw(20) << appName
                     << setw(12) << pwr
                     << setw(12) << statusBuf
                     << timeBuf << "\n";
            }
            cout << "----------------------------------------------------------------------\n";
        }
    }
    mysql_stmt_close(stmt);
}

void loadLogMenu(MYSQL* conn) {
    int choice;
    do {
        cout << "\n--- LOAD LOGS MENU ---\n";
        cout << "1. View All System Load Logs\n";
        cout << "2. View Logs by Consumer\n";
        cout << "3. Back to Main Menu\n";
        choice = (int)readNumber("Enter Choice (1-3): ", 1, 3);

        if (choice == 1) viewLoadLogs(conn);
        else if (choice == 2) {
            viewConsumers(conn);
            int cid = (int)readNumber("Enter Consumer ID: ", 1);
            viewConsumerLogs(conn, cid);
        }
    } while (choice != 3);
}
