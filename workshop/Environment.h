#pragma once
#include <cmath>
#include <string>

class DatabaseManager;

class Environment {
public:
    // default
    std::string climateType;
    double ambientTempC = 15.0;
    double pressurePa = 101325.0;

    double roadGradient = 0.0;
    double surfaceRoughness = 0.012;

    void loadEnvironment(const std::string& tType, const std::string& cType, DatabaseManager* db);

    double getAirDensity() const;
    void setRawEnvironment(double grad, double rough, double temp);
};