#include "Cost.h"
#include "Database_Manager.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <mysql.h>

// Initialize static members
double Cost::fuelPrice = 2.0; // Default price: RM 2.00 per liter
bool Cost::initialized = false;

// Constructor with DatabaseManager
Cost::Cost(DatabaseManager* dbManager) : db(dbManager) {
    if (!initialized && db) {
        loadFuelPriceFromDatabase();
        initialized = true;
    }
}

// Default constructor
Cost::Cost() : db(nullptr) {
    // Default constructor doesn't load from database
}

double Cost::calculate(double km_per_liter) const {
    if (km_per_liter <= 0) {
        return 0.0;
    }
    return fuelPrice / km_per_liter; // RM per km
}

double Cost::calculateTotalCost(double fuel_liters) const {
    return fuel_liters * fuelPrice;
}

void Cost::setFuelPrice(double price) {
    if (price > 0) {
        fuelPrice = price;
        std::cout << "Fuel price updated to RM " << std::fixed << std::setprecision(2) << price << " per liter.\n";

        // Note: Cannot save to database from static method without a Cost instance
        // The saving will need to be done elsewhere (e.g., in System class)
    }
    else {
        std::cerr << "Error: Fuel price must be positive.\n";
    }
}

double Cost::getFuelPrice() {
    return fuelPrice;
}

bool Cost::loadFuelPriceFromDatabase() {
    if (!db || !db->getConnection()) {
        std::cerr << "Database connection not available. Using default fuel price.\n";
        return false;
    }

    MYSQL* conn = db->getConnection();

    // Query to get fuel price
    std::string query = "SELECT price FROM fuel_prices ORDER BY update_time DESC LIMIT 1";

    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
        return false;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) {
        std::cerr << "No result set: " << mysql_error(conn) << std::endl;
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        fuelPrice = std::stod(row[0]);
        std::cout << "Loaded fuel price from database: RM " << std::fixed << std::setprecision(2) << fuelPrice << "\n";
    }

    mysql_free_result(result);
    return true;
}

bool Cost::saveFuelPriceToDatabase() const {
    if (!db || !db->getConnection()) {
        std::cerr << "Database connection not available. Fuel price not saved.\n";
        return false;
    }

    MYSQL* conn = db->getConnection();

    // Insert new fuel price record
    std::ostringstream oss;
    oss << "INSERT INTO fuel_prices (price) VALUES (" << std::fixed << std::setprecision(2) << fuelPrice << ")";
    std::string query = oss.str();

    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Insert failed: " << mysql_error(conn) << std::endl;
        return false;
    }

    std::cout << "Fuel price saved to database: RM " << std::fixed << std::setprecision(2) << fuelPrice << "\n";
    return true;
}

void Cost::displayCurrentPrice() const {
    std::cout << "Current Fuel Price: RM " << std::fixed << std::setprecision(2) << fuelPrice << " per liter\n";
}

std::string Cost::getFormattedPrice() const {
    std::ostringstream oss;
    oss << "RM " << std::fixed << std::setprecision(2) << fuelPrice << "/L";
    return oss.str();
}