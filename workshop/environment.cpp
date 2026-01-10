#include "Environment.h"
#include "Database_Manager.h"
#include <mysql.h>
#include <cstring>
#include <iostream>

void Environment::setRawEnvironment(double grad, double rough, double temp) {
    this->roadGradient = grad;
    this->surfaceRoughness = rough;
    this->ambientTempC = temp;
    // Keep pressure standard if not provided by raw input
    this->pressurePa = 101325.0;
}

// Only ONE body for this function exists now
double Environment::getAirDensity() const {
    const double R_specific = 287.058;
    double T_kelvin = this->ambientTempC + 273.15;
    return this->pressurePa / (R_specific * T_kelvin);
}

void Environment::updateEnvironment(const std::string& tType, const std::string& cType, DatabaseManager* db) {
    // ... your existing MySQL logic ...
    // (Ensure you use this->pressurePa when fetching from DB)
}