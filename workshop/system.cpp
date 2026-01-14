#include "System.h"
#include "Calculation_History.h"
#include "Auth.h"
#include "Preset.h"
#include "Cost.h"
#include <iostream>
#include <string>
#include <limits>
#include <iomanip>
#include <mysql.h>

System::System(DatabaseManager* db)
    : db(db), preset(db), vehicle(db), environment(), calculator(),
    calcHistory(db),
    currentUser(""), currentRole(Auth::Role::USER) {
}

void System::runApplication() {
    system("cls");

    std::cout << "========================================\n";
    std::cout << "  TACTICAL FUEL MANAGEMENT SYSTEM\n";
    std::cout << "========================================\n";

    // Login
    Auth auth(db);

    std::cout << "\n--- Authentication Required ---\n";
    if (!auth.login(currentUser, currentRole)) {
        std::cout << "Authentication failed. Exiting...\n";
        return;
    }

    // main application
    bool running = true;
    while (running) {
        displayMainMenu();

        int choice;
        std::cout << "Selection: ";
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }

        switch (choice) {
        case 1:  // Mission Setup
            missionSetup();
            break;

        case 2:  // Vehicle Management
            manageVehicles();
            break;
        case 3: // User Profile
            updateUserProfile();
            break;
        case 4: // Calculation History
            viewCalculationHistory();
            break;
        case 5:  // User Management Menu
            if (currentRole == Auth::Role::ADMIN) {
                userManagementMenu();
            }
            else {
                std::cout << "Invalid selection. Please try again.\n";
            }
            break;
        case 6:  // Fuel
            if (currentRole == Auth::Role::ADMIN) {
                manageFuelPrice();
            }
            else {
                std::cout << "Invalid selection: Please try again.\n";
            }
            break;
        case 99: // Exit
            std::cout << "Exiting system... Goodbye!\n";
            running = false;
            break;

        default:
            std::cout << "Invalid selection. Please try again.\n";
            break;
        }
    }
}

void System::displayMainMenu() {
    std::cout << "\n========================================\n";
    std::cout << "           MAIN MENU\n";
    std::cout << "========================================\n";

    std::cout << "1. Mission Setup & Calculation\n";
    std::cout << "2. Vehicle Management\n";
    std::cout << "3. Update My Profile\n";
    std::cout << "4. Calculation History\n";
    if (currentRole == Auth::Role::ADMIN) {
        std::cout << "5. User Management\n";
        std::cout << "6. Fuel Price Management\n";
    }
    std::cout << "99. Exit\n";
    std::cout << "========================================\n";
}

// MISSION SETUP MENU
void System::missionSetup() {
    std::cout << "\n=== MISSION SETUP ===\n";
    std::cout << "1. Manual Mission Configuration\n";
    std::cout << "2. Load Mission Preset\n";
    std::cout << "3. Save Current Mission as Preset\n";
    std::cout << "4. Delete Mission Preset\n";
    std::cout << "5. List All Mission Presets\n";
    std::cout << "0. Back to Main Menu\n";
    std::cout << "Selection: ";

    int choice;
    std::cin >> choice;

    switch (choice) {
    case 1:
        runManualMission();
        break;
    case 2:
        loadMissionPreset();
        break;
    case 3:
        saveMissionPreset();
        break;
    case 4:
        deleteMissionPreset();
        break;
    case 5:
        preset.listPresets();
        break;
    default:
        break;
    }
}

void System::runManualMission() {
    std::cout << "\n--- Manual Mission Configuration ---\n";

    // Get mission name
    std::string mission_name;
    std::cout << "Mission Name (optional, for history): ";
    std::cin.ignore();
    std::getline(std::cin, mission_name);

    // Environment input
    double grad, rough, temp;
    std::cout << "> Road Gradient (e.g., 0.05 for 5%): "; std::cin >> grad;
    std::cout << "> Surface Roughness (1.0=Asphalt, 1.5=Gravel, 2.5=Mud): "; std::cin >> rough;
    std::cout << "> Ambient Temperature (Celsius): "; std::cin >> temp;

    environment.setRawEnvironment(grad, rough, temp);

    // Mission details
    double distance, speed;
    std::cout << "\n--- Mission Details ---\n";
    std::cout << "> Total Distance (km): "; std::cin >> distance;
    std::cout << "> Planned Average Speed (km/h): "; std::cin >> speed;

    // Vehicle selection
    std::string vId;
    std::cout << "\n--- Vehicle Selection ---\n";
    std::cout << "> Vehicle ID: "; std::cin >> vId;

    if (!vehicle.loadVehicle(vId)) {
        std::cout << "Vehicle not found. Please add vehicle first via Vehicle Management.\n";
        return;
    }

    // Calculate and display results
    double totalFuelLiters = calculator.calculate(vehicle, environment, distance, speed);
    calculator.displayReport(totalFuelLiters, distance);

    // Save to history
    saveCalculationToHistory(mission_name, distance, speed, totalFuelLiters);
}

void System::loadMissionPreset() {
    preset.listPresets();
    std::string pName;
    std::cout << "Enter Profile Name to Load: ";
    std::cin >> pName;

    double g, r, t;
    if (preset.loadPreset(pName, g, r, t)) {
        environment.setRawEnvironment(g, r, t);
        std::cout << "Mission profile '" << pName << "' loaded successfully.\n";

        // Continue with mission setup
        double distance, speed;
        std::cout << "\n--- Mission Details ---\n";
        std::cout << "> Total Distance (km): "; std::cin >> distance;
        std::cout << "> Planned Average Speed (km/h): "; std::cin >> speed;

        std::string vId;
        std::cout << "> Vehicle ID: "; std::cin >> vId;

        if (vehicle.loadVehicle(vId)) {
            double totalFuelLiters = calculator.calculate(vehicle, environment, distance, speed);
            calculator.displayReport(totalFuelLiters, distance);

            // Save to history
            std::string mission_name = "Preset: " + pName;
            saveCalculationToHistory(mission_name, distance, speed, totalFuelLiters);
        }
        else {
            std::cout << "Vehicle not found.\n";
        }
    }
    else {
        std::cout << "Error: Profile not found.\n";
    }
}

void System::saveMissionPreset() {
    std::string pName;
    std::cout << "Enter a unique name for this mission profile: ";
    std::cin >> pName;

    preset.savePreset(pName, environment.roadGradient,
        environment.surfaceRoughness, environment.ambientTempC);
}

void System::deleteMissionPreset() {
    preset.listPresets();
    std::string pName;
    std::cout << "Enter the name of the preset to delete: ";
    std::cin >> pName;

    preset.deletePreset(pName);
}

// VEHICLE MANAGEMENT FUNCTIONS
void System::manageVehicles() {
    std::cout << "\n=== VEHICLE MANAGEMENT ===\n";
    std::cout << "1. List All Vehicles\n";
    std::cout << "2. Add New Vehicle\n";
    std::cout << "3. Update Vehicle\n";
    std::cout << "4. Delete Vehicle\n";
    std::cout << "0. Back to Main Menu\n";
    std::cout << "Selection: ";

    int choice;
    std::cin >> choice;

    switch (choice) {
    case 1:
        vehicle.listVehicles();
        break;
    case 2:
        addNewVehicle();
        break;
    case 3:
        updateVehicleDetails();
        break;
    case 4:
        deleteVehicle();
        break;
    default:
        break;
    }
}

void System::addNewVehicle() {
    std::string id, model;
    double efficiency, mass, cd, area, power, tirePressure;
    bool hasAC;

    std::cout << "\n--- Add New Vehicle ---\n";
    std::cout << "Vehicle ID: "; std::cin >> id;

    if (vehicle.loadVehicle(id)) {
        std::cout << "Vehicle ID already exists.\n";
        return;
    }

    std::cin.ignore();
    std::cout << "Model Name: "; std::getline(std::cin, model);
    std::cout << "Base Efficiency (km/L): "; std::cin >> efficiency;
    std::cout << "Mass (kg): "; std::cin >> mass;
    std::cout << "Drag Coefficient (Cd): "; std::cin >> cd;
    std::cout << "Frontal Area (m^2): "; std::cin >> area;
    std::cout << "Engine Power (kW): "; std::cin >> power;
    std::cout << "Tire Pressure (bar, default 2.4): "; std::cin >> tirePressure;

    char acChoice;
    std::cout << "Has Air Conditioning? (y/n): "; std::cin >> acChoice;
    hasAC = (acChoice == 'y' || acChoice == 'Y');

    vehicle.addVehicle(id, model, efficiency, mass, cd, area, power, tirePressure, hasAC);
}

void System::updateVehicleDetails() {
    std::string id;
    std::cout << "\n--- Update Vehicle ---\n";
    std::cout << "Enter Vehicle ID to update: ";
    std::cin >> id;

    // First check if the vehicle exists
    if (!vehicle.loadVehicle(id)) {
        std::cout << "Vehicle not found.\n";
        return;
    }

    // Display current vehicle details for reference
    vehicle.displayVehicleDetails();

    std::cout << "\n--- Enter Updated Vehicle Details ---\n";
    std::cout << "Leave field blank (press Enter) to keep current value\n";

    std::string newModel;
    double newEfficiency, newMass, newCd, newArea, newPower, newTirePressure;
    char acChoice;
    bool newHasAC;

    std::cin.ignore(); // Clear input buffer

    // Model Name
    std::cout << "Model Name [" << vehicle.model_name << "]: ";
    std::getline(std::cin, newModel);

    // Base Efficiency
    std::string efficiencyStr;
    std::cout << "Base Efficiency (km/L) [" << vehicle.efficiency << "]: ";
    std::getline(std::cin, efficiencyStr);
    newEfficiency = efficiencyStr.empty() ? vehicle.efficiency : std::stod(efficiencyStr);

    // Mass
    std::string massStr;
    std::cout << "Mass (kg) [" << vehicle.massKg << "]: ";
    std::getline(std::cin, massStr);
    newMass = massStr.empty() ? vehicle.massKg : std::stod(massStr);

    // Drag Coefficient
    std::string cdStr;
    std::cout << "Drag Coefficient (Cd) [" << vehicle.dragCoef << "]: ";
    std::getline(std::cin, cdStr);
    newCd = cdStr.empty() ? vehicle.dragCoef : std::stod(cdStr);

    // Frontal Area
    std::string areaStr;
    std::cout << "Frontal Area (m^2) [" << vehicle.frontalArea << "]: ";
    std::getline(std::cin, areaStr);
    newArea = areaStr.empty() ? vehicle.frontalArea : std::stod(areaStr);

    // Engine Power
    std::string powerStr;
    std::cout << "Engine Power (kW) [" << vehicle.engineRatedPower << "]: ";
    std::getline(std::cin, powerStr);
    newPower = powerStr.empty() ? vehicle.engineRatedPower : std::stod(powerStr);

    // Tire Pressure
    std::string tireStr;
    std::cout << "Tire Pressure (bar) [" << vehicle.tirePressureBar << "]: ";
    std::getline(std::cin, tireStr);
    newTirePressure = tireStr.empty() ? vehicle.tirePressureBar : std::stod(tireStr);

    // Air Conditioning
    std::string acStr;
    std::cout << "Has Air Conditioning? (y/n) [" << (vehicle.hasAC ? "y" : "n") << "]: ";
    std::getline(std::cin, acStr);

    if (acStr.empty()) {
        newHasAC = vehicle.hasAC;
    }
    else {
        newHasAC = (acStr[0] == 'y' || acStr[0] == 'Y');
    }

    // Update the vehicle
    if (vehicle.updateVehicle(id, newModel, newEfficiency, newMass, newCd,
        newArea, newPower, newTirePressure, newHasAC)) {
        std::cout << "Vehicle '" << id << "' updated successfully.\n";
    }
    else {
        std::cout << "Failed to update vehicle.\n";
    }
}

void System::deleteVehicle() {
    std::string id;
    std::cout << "\n--- Delete Vehicle ---\n";
    std::cout << "Enter Vehicle ID to delete: "; std::cin >> id;

    vehicle.deleteVehicle(id);
}

// USER MANAGEMENT FUNCTIONS
void System::userManagementMenu() {
    std::cout << "\n=== USER MANAGEMENT ===\n";
    std::cout << "1. List All Users\n";
    std::cout << "2. Update User Profile\n";
    std::cout << "3. Change User Role\n";
    std::cout << "4. Delete User\n";
    std::cout << "5. Register New User\n";
    std::cout << "0. Back to Main Menu\n";
    std::cout << "Selection: ";

    int choice;
    std::cin >> choice;

    switch (choice) {
    case 1:
        listAllUsers();
        break;
    case 2:
        updateOtherUserProfile();
        break;
    case 3:
        changeUserRole();
        break;
    case 4:
        deleteUserAccount();
        break;
    case 5:
        adminRegisterUser();
        break;
    default:
        break;
    }
}

void System::adminRegisterUser() {
    Auth auth(db);

    std::cout << "\n=== ADMIN: REGISTER NEW USER ===\n";

    std::string username, password, confirmPassword;
    std::string roleStr;

    // Get username
    std::cout << "Enter username: ";
    std::cin >> username;

    // Get password
    std::cout << "Enter password: ";
    std::cin >> password;

    // Confirm password
    std::cout << "Confirm password: ";
    std::cin >> confirmPassword;

    if (password != confirmPassword) {
        std::cout << "Passwords do not match.\n";
        return;
    }

    // Get role
    std::cout << "Enter role (admin/user): ";
    std::cin >> roleStr;

    Auth::Role role = auth.stringToRole(roleStr);

    // Register the user
    if (auth.registerUser(username, password, role)) {
        std::cout << "User '" << username << "' registered successfully as "
            << Auth::roleToString(role) << ".\n";
    }
}

void System::updateUserProfile() {
    Auth auth(db);

    std::string newPassword;
    std::cout << "\n=== UPDATE MY PROFILE ===\n";
    std::cout << "Enter new password: ";
    std::cin >> newPassword;

    if (auth.updateUser(currentUser, currentRole, currentUser, newPassword)) {
        std::cout << "Password updated successfully.\n";
    }
    else {
        std::cout << "Failed to update password.\n";
    }
}

void System::updateOtherUserProfile() {
    Auth auth(db);

    std::string targetUser, newPassword;
    std::cout << "\n=== UPDATE USER PROFILE ===\n";
    std::cout << "Enter username to update: ";
    std::cin >> targetUser;
    std::cout << "Enter new password: ";
    std::cin >> newPassword;

    if (auth.updateUser(currentUser, currentRole, targetUser, newPassword)) {
        std::cout << "User profile updated successfully.\n";
    }
}

void System::deleteUserAccount() {
    Auth auth(db);

    std::string targetUser;
    std::cout << "\n=== DELETE USER ACCOUNT ===\n";
    std::cout << "Enter username to delete: ";
    std::cin >> targetUser;

    if (targetUser == currentUser) {
        std::cout << "Warning: You cannot delete your own account from here.\n";
        return;
    }

    if (auth.deleteUser(currentUser, currentRole, targetUser)) {
        std::cout << "User account deleted successfully.\n";
    }
}

void System::listAllUsers() {
    Auth auth(db);
    auth.listAllUsers(currentUser, currentRole);
}

void System::changeUserRole() {
    Auth auth(db);

    std::string targetUser, roleStr;
    std::cout << "\n=== CHANGE USER ROLE ===\n";
    std::cout << "Enter username: ";
    std::cin >> targetUser;

    if (targetUser == currentUser) {
        std::cout << "Cannot change your own role.\n";
        return;
    }

    std::cout << "Enter new role (admin/user): ";
    std::cin >> roleStr;

    Auth::Role newRole = auth.stringToRole(roleStr);

    if (auth.changeUserRole(currentUser, currentRole, targetUser, newRole)) {
        std::cout << "User role changed successfully.\n";
    }
}


// Log
void System::deleteCalculationHistoryMenu() {
    std::cout << "\n=== DELETE CALCULATION HISTORY ===\n";
    std::cout << "Are you sure you want to delete ALL your calculation history? (y/n): ";
    char confirm;
    std::cin >> confirm;

    if (confirm == 'y' || confirm == 'Y') {
        if (calcHistory.deleteAllUserCalculations(currentUser)) {
            std::cout << "All calculation history deleted successfully.\n";
        }
        else {
            std::cout << "Failed to delete calculation history.\n";
        }
    }
}

void System::displayAllUserCalculations() {
    std::cout << "\n=== ALL USER CALCULATIONS (ADMIN VIEW) ===\n";

    // Get calculations for all users
    std::vector<CalculationRecord> records = calcHistory.getRecentCalculations(100);

    if (records.empty()) {
        std::cout << "No calculations found.\n";
        return;
    }

    std::cout << std::left << std::setw(5) << "ID"
        << std::setw(15) << "Date"
        << std::setw(12) << "User"
        << std::setw(15) << "Mission"
        << std::setw(12) << "Vehicle"
        << std::setw(10) << "Distance"
        << std::setw(12) << "Fuel (L)"
        << "\n";
    std::cout << std::string(81, '-') << "\n";

    for (const auto& record : records) {
        std::cout << std::left << std::setw(5) << record.id
            << std::setw(15) << record.getFormattedDate()
            << std::setw(12) << record.username
            << std::setw(15) << (record.mission_name.length() > 14 ?
                record.mission_name.substr(0, 14) + "." : record.mission_name)
            << std::setw(12) << record.vehicle_id
            << std::setw(10) << std::fixed << std::setprecision(1) << record.distance_km
            << std::setw(12) << std::fixed << std::setprecision(2) << record.fuel_consumed_liters
            << "\n";
    }
}

void System::exportCalculationHistory() {
    std::cout << "\n=== EXPORT CALCULATION HISTORY ===\n";
    std::string filename;
    std::cout << "Enter filename (without extension): ";
    std::cin >> filename;

    if (calcHistory.exportToCSV(currentUser, filename + ".csv")) {
        std::cout << "Calculation history exported to " << filename << ".csv\n";
    }
    else {
        std::cout << "Failed to export calculation history.\n";
    }
}

// FUEL PRICE MANAGEMENT FUNCTIONS
void System::manageFuelPrice() {
    int choice;

    do {
        std::cout << "\n=== FUEL PRICE MANAGEMENT ===\n";
        std::cout << "1. Display Current Fuel Price\n";
        std::cout << "2. Update Fuel Price\n";
        std::cout << "0. Back to Main Menu\n";
        std::cout << "Selection: ";
        std::cin >> choice;

        switch (choice) {
        case 1:
            displayFuelPrice();
            break;
        case 2:
            updateFuelPrice();
            break;
        case 0:
            return;
        default:
            std::cout << "Invalid selection. Please try again.\n";
        }
    } while (choice != 0);
}

void System::updateFuelPrice() {
    std::cout << "\n=== UPDATE FUEL PRICE ===\n";

    Cost costCalculator(db); // Pass the DatabaseManager to Cost constructor
    costCalculator.displayCurrentPrice();

    double newPrice;
    std::cout << "\nEnter new fuel price (RM per liter): ";
    std::cin >> newPrice;

    if (newPrice <= 0) {
        std::cout << "Error: Fuel price must be positive.\n";
        return;
    }

    // Confirm update
    std::cout << "Are you sure you want to change fuel price to RM "
        << std::fixed << std::setprecision(2) << newPrice
        << " per liter? (y/n): ";

    char confirm;
    std::cin >> confirm;

    if (confirm == 'y' || confirm == 'Y') {
        // Since setFuelPrice is now non-static, call it on the object
        costCalculator.setFuelPrice(newPrice);

        std::cout << "Fuel price updated successfully.\n";
        std::cout << "Note: All future calculations will use the new price.\n";
        std::cout << "Existing calculation history will NOT be updated.\n";
    }
    else {
        std::cout << "Update cancelled.\n";
    }
}

void System::displayFuelPrice() {
    std::cout << "\n=== CURRENT FUEL PRICE ===\n";

    Cost costCalculator(db); // Pass the DatabaseManager to Cost constructor
    costCalculator.displayCurrentPrice();
}

// CALCULATION HISTORY FUNCTIONS
void System::viewCalculationHistory() {
    std::cout << "\n=== CALCULATION HISTORY ===\n";
    std::cout << "1. View My Calculations\n";
    std::cout << "2. View Recent Calculations (All Users)\n";
    std::cout << "3. View Statistics\n";
    std::cout << "4. Export History to CSV\n";
    if (currentRole == Auth::Role::ADMIN) {
        std::cout << "5. Delete My Calculation History\n";
        std::cout << "6. View All User Calculations (Admin)\n";
    }
    std::cout << "0. Back to Main Menu\n";
    std::cout << "Selection: ";

    int choice;
    std::cin >> choice;

    switch (choice) {
    case 1:
        displayUserCalculations();
        break;
    case 2:
        displayRecentCalculations();
        break;
    case 3:
        displayCalculationStatistics();
        break;
    case 4:
        exportCalculationHistory();
        break;
    case 5:
        if (currentRole == Auth::Role::ADMIN) {
            deleteCalculationHistoryMenu();
        }
        break;
    case 6:
        if (currentRole == Auth::Role::ADMIN) {
            displayAllUserCalculations();
        }
        break;
    default:
        break;
    }
}

void System::saveCalculationToHistory(const std::string& mission_name, double distance, double speed, double fuel_consumed) {
    CalculationRecord record;

    // Basic info
    record.username = currentUser;
    record.vehicle_id = vehicle.vehicle_id;
    record.mission_name = mission_name.empty() ? "Unnamed Mission" : mission_name;

    // Vehicle parameters
    record.vehicle_mass = vehicle.massKg;
    record.vehicle_drag_coef = vehicle.dragCoef;
    record.vehicle_frontal_area = vehicle.frontalArea;
    record.vehicle_tire_pressure = vehicle.tirePressureBar;
    record.vehicle_engine_power = vehicle.engineRatedPower;
    record.vehicle_has_ac = vehicle.hasAC;
    record.vehicle_efficiency = vehicle.efficiency;

    // Environmental parameters
    record.road_gradient = environment.roadGradient;
    record.surface_roughness = environment.surfaceRoughness;
    record.ambient_temp = environment.ambientTempC;
    record.pressure = environment.pressurePa;

    // Mission parameters
    record.distance_km = distance;
    record.avg_speed_kmh = speed;

    // Results
    record.fuel_consumed_liters = fuel_consumed;

    // Calculate cost per km
    Cost costCalculator(db); // Pass the DatabaseManager to Cost constructor
    if (distance > 0) {
        double km_per_l = distance / fuel_consumed;
        record.cost_per_km = costCalculator.calculate(km_per_l);
    }
    else {
        record.cost_per_km = 0.0;
    }

    // Save to database
    if (calcHistory.saveCalculation(record)) {
        // Successfully saved
    }
    else {
        std::cerr << "Warning: Could not save calculation to history.\n";
    }
}

void System::displayUserCalculations() {
    std::vector<CalculationRecord> records = calcHistory.getUserCalculations(currentUser, 50);

    if (records.empty()) {
        std::cout << "\nNo calculation history found for user: " << currentUser << "\n";
        return;
    }

    std::cout << "\n=== My Calculation History ===\n";
    std::cout << "Total calculations: " << records.size() << "\n\n";

    std::cout << std::left << std::setw(5) << "ID"
        << std::setw(15) << "Date"
        << std::setw(15) << "Mission"
        << std::setw(12) << "Vehicle"
        << std::setw(10) << "Distance"
        << std::setw(10) << "Speed"
        << std::setw(12) << "Fuel (L)"
        << std::setw(12) << "Cost/km"
        << "\n";
    std::cout << std::string(91, '-') << "\n";

    for (const auto& record : records) {
        std::cout << std::left << std::setw(5) << record.id
            << std::setw(15) << record.getFormattedDate()
            << std::setw(15) << (record.mission_name.length() > 14 ?
                record.mission_name.substr(0, 14) + "." : record.mission_name)
            << std::setw(12) << (record.vehicle_id.length() > 11 ?
                record.vehicle_id.substr(0, 11) + "." : record.vehicle_id)
            << std::setw(10) << std::fixed << std::setprecision(1) << record.distance_km
            << std::setw(10) << std::fixed << std::setprecision(1) << record.avg_speed_kmh
            << std::setw(12) << std::fixed << std::setprecision(2) << record.fuel_consumed_liters
            << std::setw(12) << std::fixed << std::setprecision(3) << record.cost_per_km
            << "\n";
    }

    // Show summary
    double total_fuel = 0;
    double total_distance = 0;
    for (const auto& record : records) {
        total_fuel += record.fuel_consumed_liters;
        total_distance += record.distance_km;
    }

    std::cout << "\n--- Summary ---\n";
    std::cout << "Total Fuel Consumed: " << std::fixed << std::setprecision(2) << total_fuel << " L\n";
    std::cout << "Total Distance: " << std::fixed << std::setprecision(1) << total_distance << " km\n";
    if (total_distance > 0) {
        std::cout << "Average Fuel Efficiency: " << std::fixed << std::setprecision(2)
            << (total_distance / total_fuel) << " km/L\n";
        std::cout << "Total Fuel Cost: RM " << std::fixed << std::setprecision(2)
            << (total_fuel * 2.0) << "\n";
    }
}

void System::displayRecentCalculations() {
    std::vector<CalculationRecord> records = calcHistory.getRecentCalculations(20);

    if (records.empty()) {
        std::cout << "\nNo recent calculations found.\n";
        return;
    }

    std::cout << "\n=== Recent Calculations (All Users) ===\n";
    std::cout << std::left << std::setw(5) << "ID"
        << std::setw(15) << "Date"
        << std::setw(10) << "User"
        << std::setw(12) << "Vehicle"
        << std::setw(10) << "Distance"
        << std::setw(10) << "Fuel (L)"
        << "\n";
    std::cout << std::string(62, '-') << "\n";

    for (const auto& record : records) {
        std::cout << std::left << std::setw(5) << record.id
            << std::setw(15) << record.getFormattedDate()
            << std::setw(10) << record.username
            << std::setw(12) << (record.vehicle_id.length() > 11 ?
                record.vehicle_id.substr(0, 11) + "." : record.vehicle_id)
            << std::setw(10) << std::fixed << std::setprecision(1) << record.distance_km
            << std::setw(10) << std::fixed << std::setprecision(2) << record.fuel_consumed_liters
            << "\n";
    }
}

void System::displayCalculationStatistics() {
    int user_count = calcHistory.getCalculationCount(currentUser);
    int total_count = calcHistory.getCalculationCount();

    std::cout << "\n=== Calculation Statistics ===\n";
    std::cout << "Your calculations: " << user_count << "\n";
    std::cout << "Total system calculations: " << total_count << "\n";

    if (user_count > 0) {
        std::vector<CalculationRecord> records = calcHistory.getUserCalculations(currentUser, 1000);

        double total_fuel = 0;
        double total_distance = 0;
        double max_fuel = 0;
        double min_fuel = (records.size() > 0) ? records[0].fuel_consumed_liters : 0;

        for (const auto& record : records) {
            total_fuel += record.fuel_consumed_liters;
            total_distance += record.distance_km;
            if (record.fuel_consumed_liters > max_fuel) max_fuel = record.fuel_consumed_liters;
            if (record.fuel_consumed_liters < min_fuel) min_fuel = record.fuel_consumed_liters;
        }

        std::cout << "\n--- Your Performance ---\n";
        std::cout << "Total fuel consumed: " << std::fixed << std::setprecision(2) << total_fuel << " L\n";
        std::cout << "Total distance: " << std::fixed << std::setprecision(1) << total_distance << " km\n";
        if (total_distance > 0) {
            std::cout << "Average efficiency: " << std::fixed << std::setprecision(2)
                << (total_distance / total_fuel) << " km/L\n";
            std::cout << "Most fuel-intensive mission: " << std::fixed << std::setprecision(2)
                << max_fuel << " L\n";
            std::cout << "Most efficient mission: " << std::fixed << std::setprecision(2)
                << min_fuel << " L\n";
        }
    }
}