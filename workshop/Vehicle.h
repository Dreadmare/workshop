#pragma once
#include <string>
#include <mysql.h>

class DatabaseManager;

class Vehicle {
public:
    // Physical Attributes
    std::string vehicle_id;
    std::string model_name;
    double massKg;
    double dragCoef;
    double frontalArea;
    double tirePressureBar;
    double engineRatedPower;
    double efficiency;
    bool hasAC;

    DatabaseManager* db;

    Vehicle(DatabaseManager* db);
    Vehicle(std::string id, double mass, double cd, double area, double power);

    ~Vehicle();

    // Database Operations
    bool addVehicle(const std::string& id, const std::string& model, double efficiency,double mass, double cd, double area, double power, double tirePressure = 2.4, bool ac = false);
    bool updateVehicle(const std::string& id, const std::string& model, double efficiency, double mass, double cd, double area, double power, double tirePressure, bool ac);

    bool deleteVehicle(const std::string& id);

    void listVehicles();
    bool loadVehicle(const std::string& id);

    // Utility Methods
    bool vehicleExists(const std::string& id);
    void displayVehicleDetails() const;

    // Setters
    void setTirePressure(double pressure) { tirePressureBar = pressure; }
    void setHasAC(bool acStatus) { hasAC = acStatus; }
    void setEfficiency(double newEfficiency) { efficiency = newEfficiency; }

    //debug
    //void debugCheckVehicle(const std::string& id);
};