#ifndef CONSUMER_H
#define CONSUMER_H

#include <winsock2.h>
#include <mysql.h>
#include <string>
#include <vector>

struct Consumer {
    int id;
    std::string name;
    std::string type;
    std::string contact;
    std::string address;
    float capacity;
};

int addConsumer(MYSQL* conn);
void viewConsumers(MYSQL* conn);
void updateConsumer(MYSQL* conn);
void deleteConsumer(MYSQL* conn);
Consumer getConsumerById(MYSQL* conn, int consumerId);
bool consumerExists(MYSQL* conn, int consumerId);
void consumerMenu(MYSQL* conn);

#endif // CONSUMER_H
