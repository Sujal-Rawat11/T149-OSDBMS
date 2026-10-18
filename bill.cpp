#include "bill.h"
#include "consumer.h"
#include "helpers.h"
#include <iostream>
#include <iomanip>
#include <cstring>

using namespace std;

void generateBill(MYSQL* conn) {
    viewConsumers(conn);
    int consumerId = (int)readNumber("Select Consumer ID to generate bill: ", 1);
    if (!consumerExists(conn, consumerId)) {
        cout << "Consumer not found.\n";
        return;
    }

    float units = readNumber("Enter Electricity Units Consumed (kWh): ", 0);
    float rate = readNumber("Enter Rate per Unit ($/kWh): ", 0.01f);
    float total = units * rate;

    const char* query = "INSERT INTO bills (consumer_id, units_consumed, rate_per_unit, total_amount, payment_status) VALUES (?, ?, ?, ?, 'UNPAID')";
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return;

    if (!mysql_stmt_prepare(stmt, query, strlen(query))) {
        MYSQL_BIND bind[4];
        memset(bind, 0, sizeof(bind));

        bind[0].buffer_type = MYSQL_TYPE_LONG; bind[0].buffer = &consumerId;
        bind[1].buffer_type = MYSQL_TYPE_FLOAT; bind[1].buffer = &units;
        bind[2].buffer_type = MYSQL_TYPE_FLOAT; bind[2].buffer = &rate;
        bind[3].buffer_type = MYSQL_TYPE_FLOAT; bind[3].buffer = &total;

        if (!mysql_stmt_bind_param(stmt, bind) && !mysql_stmt_execute(stmt)) {
            int billId = (int)mysql_stmt_insert_id(stmt);
            cout << "\n========================================\n";
            cout << "            BILL GENERATED\n";
            cout << "========================================\n";
            cout << "Bill ID         : " << billId << "\n";
            cout << "Consumer ID     : " << consumerId << "\n";
            cout << "Units Consumed  : " << units << " kWh\n";
            cout << "Rate per Unit   : $" << fixed << setprecision(2) << rate << "\n";
            cout << "Total Amount    : $" << total << "\n";
            cout << "Payment Status  : UNPAID\n";
            cout << "========================================\n";
        } else {
            cerr << "Failed to generate bill: " << mysql_stmt_error(stmt) << "\n";
        }
    }
    mysql_stmt_close(stmt);
}

void viewBills(MYSQL* conn) {
    cout << "\n===================================================================================================\n";
    cout << "                                         ALL ELECTRICITY BILLS\n";
    cout << "===================================================================================================\n";

    const char* query = 
        "SELECT b.bill_id, c.name, b.units_consumed, b.rate_per_unit, b.total_amount, b.billing_date, b.payment_status "
        "FROM bills b JOIN consumers c ON b.consumer_id = c.id ORDER BY b.billing_date DESC";

    if (mysql_query(conn, query)) {
        cerr << "Query failed: " << mysql_error(conn) << "\n";
        return;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return;

    cout << left << setw(8) << "Bill ID"
         << setw(20) << "Consumer"
         << setw(14) << "Units (kWh)"
         << setw(12) << "Rate ($)"
         << setw(14) << "Total ($)"
         << setw(20) << "Date"
         << "Status\n";
    cout << "---------------------------------------------------------------------------------------------------\n";

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        cout << left << setw(8) << (row[0] ? row[0] : "")
             << setw(20) << (row[1] ? row[1] : "")
             << setw(14) << (row[2] ? row[2] : "")
             << setw(12) << (row[3] ? row[3] : "")
             << setw(14) << (row[4] ? row[4] : "")
             << setw(20) << (row[5] ? row[5] : "")
             << (row[6] ? row[6] : "") << "\n";
    }
    cout << "---------------------------------------------------------------------------------------------------\n";
    mysql_free_result(res);
}

void viewConsumerBills(MYSQL* conn, int consumerId) {
    cout << "\n--- BILLS FOR CONSUMER ID " << consumerId << " ---\n";
    const char* query = 
        "SELECT bill_id, units_consumed, rate_per_unit, total_amount, billing_date, payment_status "
        "FROM bills WHERE consumer_id = ? ORDER BY billing_date DESC";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return;

    if (!mysql_stmt_prepare(stmt, query, strlen(query))) {
        MYSQL_BIND bind[1];
        memset(bind, 0, sizeof(bind));
        bind[0].buffer_type = MYSQL_TYPE_LONG; bind[0].buffer = &consumerId;

        if (!mysql_stmt_bind_param(stmt, bind) && !mysql_stmt_execute(stmt)) {
            MYSQL_BIND resBind[6];
            memset(resBind, 0, sizeof(resBind));

            int billId = 0;
            float units = 0.0f, rate = 0.0f, total = 0.0f;
            char dateBuf[51] = {0}, statusBuf[21] = {0};

            resBind[0].buffer_type = MYSQL_TYPE_LONG; resBind[0].buffer = &billId;
            resBind[1].buffer_type = MYSQL_TYPE_FLOAT; resBind[1].buffer = &units;
            resBind[2].buffer_type = MYSQL_TYPE_FLOAT; resBind[2].buffer = &rate;
            resBind[3].buffer_type = MYSQL_TYPE_FLOAT; resBind[3].buffer = &total;
            resBind[4].buffer_type = MYSQL_TYPE_STRING; resBind[4].buffer = dateBuf; resBind[4].buffer_length = 50;
            resBind[5].buffer_type = MYSQL_TYPE_STRING; resBind[5].buffer = statusBuf; resBind[5].buffer_length = 20;

            mysql_stmt_bind_result(stmt, resBind);
            cout << left << setw(8) << "Bill ID"
                 << setw(14) << "Units (kWh)"
                 << setw(12) << "Rate ($)"
                 << setw(14) << "Total ($)"
                 << setw(20) << "Date"
                 << "Status\n";
            cout << "-----------------------------------------------------------------------------------\n";

            while (!mysql_stmt_fetch(stmt)) {
                cout << left << setw(8) << billId
                     << setw(14) << units
                     << setw(12) << fixed << setprecision(2) << rate
                     << setw(14) << total
                     << setw(20) << dateBuf
                     << statusBuf << "\n";
            }
            cout << "-----------------------------------------------------------------------------------\n";
        }
    }
    mysql_stmt_close(stmt);
}

void updatePaymentStatus(MYSQL* conn) {
    viewBills(conn);
    int billId = (int)readNumber("Enter Bill ID to update payment status: ", 1);
    cout << "Select Status:\n1. PAID\n2. UNPAID\n";
    int sChoice = (int)readNumber("Choice (1-2): ", 1, 2);
    string newStatus = (sChoice == 1) ? "PAID" : "UNPAID";

    const char* query = "UPDATE bills SET payment_status=? WHERE bill_id=?";
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return;

    if (!mysql_stmt_prepare(stmt, query, strlen(query))) {
        MYSQL_BIND bind[2];
        memset(bind, 0, sizeof(bind));

        bind[0].buffer_type = MYSQL_TYPE_STRING; bind[0].buffer = (void*)newStatus.c_str(); bind[0].buffer_length = newStatus.length();
        bind[1].buffer_type = MYSQL_TYPE_LONG; bind[1].buffer = &billId;

        if (!mysql_stmt_bind_param(stmt, bind) && !mysql_stmt_execute(stmt)) {
            cout << "Payment status updated to " << newStatus << " for Bill ID " << billId << ".\n";
        } else {
            cerr << "Failed to update status.\n";
        }
    }
    mysql_stmt_close(stmt);
}

void billMenu(MYSQL* conn) {
    int choice;
    do {
        cout << "\n--- ELECTRICITY BILLING MENU ---\n";
        cout << "1. Generate Bill\n";
        cout << "2. View All Bills\n";
        cout << "3. View Bills by Consumer\n";
        cout << "4. Update Payment Status\n";
        cout << "5. Back to Main Menu\n";
        choice = (int)readNumber("Enter Choice (1-5): ", 1, 5);

        if (choice == 1) generateBill(conn);
        else if (choice == 2) viewBills(conn);
        else if (choice == 3) {
            viewConsumers(conn);
            int cid = (int)readNumber("Enter Consumer ID: ", 1);
            viewConsumerBills(conn, cid);
        }
        else if (choice == 4) updatePaymentStatus(conn);
    } while (choice != 5);
}
