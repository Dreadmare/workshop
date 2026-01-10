#include "Calculator.h"
#include "Vehicle.h"
#include "Environment.h"
#include <cmath>
#include <algorithm>
#include <iostream>

double Calculator::calculate(Vehicle& veh, Environment& env, double distanceKm, double avgSpeedKmh) {

    // 1. Convert units to SI
    double v = avgSpeedKmh / 3.6; // m/s
    double durationSec = (distanceKm * 1000.0) / v;
    double g = 9.81;
    double rho = env.getAirDensity();

    double C_rr = env.surfaceRoughness * std::pow(veh.tirePressureBar, -0.477);
    double F_roll = C_rr * veh.massKg * g * std::cos(std::atan(env.roadGradient));

    double F_aero = 0.5 * rho * veh.dragCoef * veh.frontalArea * std::pow(v, 2);

    double F_grade = veh.massKg * g * std::sin(std::atan(env.roadGradient));

    double F_total = F_roll + F_aero + F_grade;
    double P_wheels = (std::max)(0.0, F_total * v);

    double P_aux = 300.0;
    if (veh.hasAC && env.ambientTempC > 20.0) {
        P_aux += 4000.0;
    }

    double P_required = (P_wheels / 0.85) + P_aux;
    double load_factor = P_required / (veh.engineRatedPower * 1000.0);

    double x = std::clamp(load_factor, 0.2, 1.0);
    double efficiency = 0.5968 * x - 0.1666 * pow(x, 2) + 2.4968 * pow(x, 3) - 2.1128 + 0.4;

    efficiency = std::clamp(efficiency, 0.30, 0.45);

    double totalEnergyJoule = P_required * durationSec;
    double fuelMassKg = totalEnergyJoule / (43000000.0 * efficiency);

    return fuelMassKg / 0.832;
}

void Calculator::displayReport(double finalEfficiency, double distanceKm) {
    std::cout << "\n--- Mission Report ---" << std::endl;
    std::cout << "Fuel Consumed: " << finalEfficiency << " L" << std::endl;
    std::cout << "Total Distance: " << distanceKm << " km" << std::endl;
}