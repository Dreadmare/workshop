#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <string>
#include "Vehicle.h"
#include "Environment.h"

class Calculator {
public:
    double calculate(Vehicle& vehicle, Environment& environment, double distanceKm, double avgSpeedKmh);

    void displayReport(double finalEfficiency, double distanceKm);
};

#endif