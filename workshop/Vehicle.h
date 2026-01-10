#pragma once
#include <string>
#include <mysql.h>

class DatabaseManager;

class Vehicle {
public:
    std::string id;
    double massKg;
    double dragCoef;
    double frontalArea;
    double tirePressureBar;
    double engineRatedPower;
    bool hasAC;

    DatabaseManager* db;

    Vehicle(DatabaseManager* db);
    Vehicle(std::string id, double mass, double cd, double area, double power);

    void addVehicle(const std::string& id, const std::string& model, double efficiency);
    void listVehicles();
    double getVehicleEfficiency(const std::string& id);
};