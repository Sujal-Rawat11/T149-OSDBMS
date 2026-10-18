#ifndef LOAD_LOG_H
#define LOAD_LOG_H

#include <winsock2.h>
#include <mysql.h>
#include <string>

void dbInsertLoadLog(MYSQL* conn, int consumerId, int applianceId, float powerWatts, const std::string& status);
void viewLoadLogs(MYSQL* conn);
void viewConsumerLogs(MYSQL* conn, int consumerId);
void loadLogMenu(MYSQL* conn);

#endif // LOAD_LOG_H
