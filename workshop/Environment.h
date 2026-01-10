#pragma once
#include <cmath>
#include <string>

class DatabaseManager;

class Environment {
public:
    std::string climateType;
    double ambientTempC = 15.0; // Default to ISA Standard
    double pressurePa = 101325.0;

    double roadGradient = 0.0;
    double surfaceRoughness = 0.012;

    void updateEnvironment(const std::string& tType, const std::string& cType, DatabaseManager* db);

    // FIX: Removed the { body } from here to prevent redefinition error
    double getAirDensity() const;

    void setRawEnvironment(double grad, double rough, double temp);
};