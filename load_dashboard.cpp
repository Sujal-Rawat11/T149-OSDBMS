#include "load_dashboard.h"
#include "consumer.h"
#include "appliance.h"
#include "supply_source.h"
#include "demand_record.h"
#include "helpers.h"
#include <iostream>
#include <iomanip>

using namespace std;

void showLoadDashboard(MYSQL* conn, int consumerId) {
    Consumer c = getConsumerById(conn, consumerId);
    if (c.id == -1) {
        cout << "Invalid Consumer ID.\n";
        return;
    }

    vector<Appliance> list = loadConsumerAppliances(conn, consumerId);
    
    // Balance load based on consumer capacity
    balanceLoad(list, c.capacity);
    dbSyncStatuses(conn, consumerId, list);

    float currentLoad = usedPower(list);
    float totalDemand = demandOf(list);

    // Total available capacity from active supply sources
    float supplyCapacity = getTotalActiveCapacity(conn);
    float effectiveCapacity = (supplyCapacity > 0) ? supplyCapacity : c.capacity;

    float remaining = effectiveCapacity - currentLoad;

    cout << "\n========================================\n";
    cout << "              LOAD STATUS               \n";
    cout << "========================================\n";
    cout << "Consumer Name      : " << c.name << " (" << c.type << ")\n";
    cout << "Available Capacity : " << effectiveCapacity << " W\n";
    cout << "Current Load       : " << currentLoad << " W\n";
    cout << "Total Demand       : " << totalDemand << " W\n";
    cout << "Remaining Capacity : " << (remaining < 0 ? 0 : remaining) << " W\n\n";

    string status = (currentLoad <= effectiveCapacity) ? "SAFE" : "OVERLOAD WARNING";
    cout << "Status: " << status << "\n";
    cout << "========================================\n";

    if (currentLoad <= effectiveCapacity) {
        cout << "Load is within safe limit.\n";
    } else {
        cout << "WARNING: Load exceeds available capacity.\n";
    }

    cout << "\nACTIVE APPLIANCES:\n";
    cout << left << setw(4) << "No." << setw(20) << "Appliance" << setw(12) << "Power(W)" << "Priority\n";
    cout << "---------------------------------------------------\n";
    int count = 1;
    for (const auto& a : list) {
        if (a.on) {
            cout << left << setw(4) << count++
                 << setw(20) << a.name
                 << setw(12) << a.power
                 << priorityLabel(a.priority) << "\n";
        }
    }
    cout << "---------------------------------------------------\n";

    // Store demand record automatically
    recordDemand(conn, consumerId, currentLoad, effectiveCapacity, totalDemand);
}

void checkCurrentLoadMenu(MYSQL* conn) {
    viewConsumers(conn);
    int cid = (int)readNumber("Select Consumer ID to check load status: ", 1);
    showLoadDashboard(conn, cid);
}
