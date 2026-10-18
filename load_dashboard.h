#ifndef LOAD_DASHBOARD_H
#define LOAD_DASHBOARD_H

#include <winsock2.h>
#include <mysql.h>

void showLoadDashboard(MYSQL* conn, int consumerId);
void checkCurrentLoadMenu(MYSQL* conn);

#endif // LOAD_DASHBOARD_H
