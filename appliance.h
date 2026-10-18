#ifndef APPLIANCE_H
#define APPLIANCE_H

#include <winsock2.h>
#include <mysql.h>
#include <string>
#include <vector>

struct Appliance {
    int dbId;       // database primary key
    std::string name;
    float power;    // Watts
    int priority;   // 1 = HIGH ... 5 = LOW
    bool on;        // state
};

// Original algorithms preserved
int autoPriority(const std::string& type, std::string name);
int askPriority(const std::string& type, const std::string& name);
float demandOf(const std::vector<Appliance>& list);
float usedPower(const std::vector<Appliance>& list);
void shedLoad(std::vector<Appliance>& list, float capacity);
void restoreLoad(std::vector<Appliance>& list, float capacity);
void balanceLoad(std::vector<Appliance>& list, float capacity);
std::string warningLevel(float used, float capacity);
bool essentialShed(const std::vector<Appliance>& list);

// Database helpers for appliances
void dbSaveAppliance(MYSQL* conn, int consumerId, Appliance& a);
void dbDeleteAppliance(MYSQL* conn, int dbId);
void dbUpdatePriority(MYSQL* conn, int dbId, int priority);
void dbUpdateApplianceStatus(MYSQL* conn, int dbId, bool isOn);
void dbSyncStatuses(MYSQL* conn, int consumerId, const std::vector<Appliance>& list);
std::vector<Appliance> loadConsumerAppliances(MYSQL* conn, int consumerId);

// Appliance Menu & CRUD
void addApplianceInteractive(MYSQL* conn, int consumerId);
void viewAppliances(MYSQL* conn, int consumerId);
void turnApplianceOnOff(MYSQL* conn, int consumerId);
void applianceMenu(MYSQL* conn);

#endif // APPLIANCE_H
