#include "System.h"
#include "Auth.h"
#include "Environment.h"
#include <iostream>
#include <string>

System::System(DatabaseManager* db)
    : db(db), preset(db), vehicle(db), environment(), calculator() {
}

void System::login() {
    std::string loggedInUser;
    Auth auth(db);
    std::cout << "--- Tactical Fuel Management System ---\n";
    if (!auth.login(loggedInUser)) {
        std::cout << "Unauthorized access attempt. System Locking.\n";
        exit(0);
    }
}

bool System::run() {
    std::string vId;
    double distance, speed;
    int choice;

    std::cout << "\n1. Manual Physics Entry\n2. Load Preset (Keywords)\n99. Exit\nSelection: ";
    if (!(std::cin >> choice)) return false;
    if (choice == 99) return false;

    if (choice == 2) {
        preset.listPresets();
        std::string pName, tType, cType;
        std::cout << "Enter Preset Name: ";
        std::cin >> pName;
        if (preset.loadPreset(pName, tType, cType)) {
            environment.updateEnvironment(tType, cType, db);
        }
        else {
            std::cout << "Preset not found. Defaulting to manual.\n";
            choice = 1;
        }
    }

    if (choice == 1) {
        double grad, rough, temp;
        std::cout << "\n--- Environmental Raw Data ---\n";
        std::cout << "Enter Road Gradient (e.g., 0.02 for 2% incline): ";
        std::cin >> grad;
        std::cout << "Enter Surface Roughness (1.0: Smooth, 1.5: Gravel, 2.5: Mud): ";
        std::cin >> rough;
        std::cout << "Enter Ambient Temperature (Celsius): ";
        std::cin >> temp;

        // Directly set the data without a DB lookup
        environment.setRawEnvironment(grad, rough, temp);
    }

    std::cout << "\nEnter Vehicle ID: ";
    std::cin >> vId;
    double baseEfficiency = vehicle.getVehicleEfficiency(vId);

    if (baseEfficiency <= 0) {
        std::string model;
        std::cout << "Vehicle ID not found. Efficiency (km/L): ";
        std::cin >> baseEfficiency;
        std::cout << "Model: ";
        std::cin.ignore();
        std::getline(std::cin, model);
        vehicle.addVehicle(vId, model, baseEfficiency);
    }

    std::cout << "Mission Distance (km): ";
    std::cin >> distance;
    std::cout << "Average Speed (km/h): ";
    std::cin >> speed;

    double fuelUsed = calculator.calculate(vehicle, environment, distance, speed);
    calculator.displayReport(fuelUsed, distance);

    return true;
}