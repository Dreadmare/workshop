#ifndef SYSTEM_H
#define SYSTEM_H

#include "Database_Manager.h"
#include "Preset.h"
#include "Vehicle.h"
#include "Environment.h"
#include "Calculator.h"
#include "Auth.h"
#include "Calculation_History.h"

class System {
public:
    System(DatabaseManager* db);

    void runApplication();

    void userManagementMenu();
    void missionSetup();

    // Calculation History Functions
    void viewCalculationHistory();
    void saveCalculationToHistory(const std::string& mission_name, double distance, double speed, double fuel_consumed);

private:
    DatabaseManager* db;
    Preset preset;
    Vehicle vehicle;
    Environment environment;
    Calculator calculator;
    CalculationHistory calcHistory;

    // Current user info
    std::string currentUser;
    Auth::Role currentRole;

    // Helper functions
    void displayMainMenu();
    void updateUserProfile();
    void updateOtherUserProfile();
    void deleteUserAccount();
    void listAllUsers();
    void changeUserRole();
    void adminRegisterUser();

    // Mission functions
    void runManualMission();
    void loadMissionPreset();
    void saveMissionPreset();
    void deleteMissionPreset();

    // Vehicle functions
    void manageVehicles();
    void addNewVehicle();
    void updateVehicleDetails();
    void deleteVehicle();

    // Calculation History helper functions
    void displayUserCalculations();
    void displayRecentCalculations();
    void displayCalculationStatistics();
    void deleteCalculationHistoryMenu();
    void displayAllUserCalculations();
    void exportCalculationHistory();

    void manageFuelPrice();
    void updateFuelPrice();
    void displayFuelPrice();
};

#endif