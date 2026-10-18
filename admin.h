#ifndef ADMIN_H
#define ADMIN_H

#include <winsock2.h>
#include <mysql.h>

bool adminLogin(MYSQL* conn);
void adminMenu(MYSQL* conn);

#endif // ADMIN_H
