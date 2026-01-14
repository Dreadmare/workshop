#ifndef COST_H
#define COST_H

#include <string>

class DatabaseManager;

class Cost {
private:
    static double fuelPrice; // RM per liter
    static bool initialized;
    DatabaseManager* db;

public:
    // Constructors
    Cost(DatabaseManager* dbManager = nullptr);
    Cost();

    double calculate(double km_per_liter) const;

    double calculateTotalCost(double fuel_liters) const;

    static void setFuelPrice(double price);
    static double getFuelPrice();
    bool loadFuelPriceFromDatabase();
    bool saveFuelPriceToDatabase() const;
    void displayCurrentPrice() const;
    std::string getFormattedPrice() const;
};

#endif