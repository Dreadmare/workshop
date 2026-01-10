#ifndef SYSTEM_H
#define SYSTEM_H

#include "Database_Manager.h"
#include "Preset.h"
#include "Vehicle.h"
#include "Environment.h"  // Use this
#include "Calculator.h"

class System {
public:
    System(DatabaseManager* db);
    void login();
    bool run();

private:
    DatabaseManager* db;
    Preset preset;
    Vehicle vehicle;
    Calculator calculator;
    Environment environment;
};

#endif