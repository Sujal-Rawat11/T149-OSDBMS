#ifndef BILL_H
#define BILL_H

#include <winsock2.h>
#include <mysql.h>
#include <string>

struct Bill {
    int billId;
    int consumerId;
    float unitsConsumed;
    float ratePerUnit;
    float totalAmount;
    std::string billingDate;
    std::string paymentStatus;
};

void generateBill(MYSQL* conn);
void viewBills(MYSQL* conn);
void viewConsumerBills(MYSQL* conn, int consumerId);
void updatePaymentStatus(MYSQL* conn);
void billMenu(MYSQL* conn);

#endif // BILL_H
