#ifndef CALCULATION_HISTORY_H
#define CALCULATION_HISTORY_H

#include <string>
#include <vector>
#include <ctime>
#include "Database_Manager.h"

struct CalculationRecord {
    int id;
    std::string username;
    std::string vehicle_id;
    std::string mission_name;

    // Vehicle parameters
    double vehicle_mass;
    double vehicle_drag_coef;
    double vehicle_frontal_area;
    double vehicle_tire_pressure;
    double vehicle_engine_power;
    bool vehicle_has_ac;
    double vehicle_efficiency;

    // Environmental parameters
    double road_gradient;
    double surface_roughness;
    double ambient_temp;
    double pressure;

    // Mission parameters
    double distance_km;
    double avg_speed_kmh;

    // Results
    double fuel_consumed_liters;
    double cost_per_km;

    // Metadata
    std::string calculated_at;

    // Helper methods
    std::string getFormattedDate() const;
    double getTotalFuelCost() const;
};

class CalculationHistory {
public:
    CalculationHistory(DatabaseManager* db);

    // CRUD Operations
    bool saveCalculation(const CalculationRecord& record);
    std::vector<CalculationRecord> getUserCalculations(const std::string& username, int limit = 50);
    std::vector<CalculationRecord> getRecentCalculations(int limit = 20);
    std::vector<CalculationRecord> getVehicleCalculations(const std::string& vehicle_id, int limit = 50);
    CalculationRecord getCalculationById(int calculation_id);
    bool deleteCalculation(int calculation_id, const std::string& requesting_user);
    bool deleteAllUserCalculations(const std::string& username);

    // Statistics
    int getCalculationCount(const std::string& username = "");
    double getTotalFuelConsumed(const std::string& username = "");
    double getAverageFuelConsumption(const std::string& username = "");

    // Export/Import
    bool exportToCSV(const std::string& username, const std::string& filename);
    std::vector<CalculationRecord> searchCalculations(const std::string& username,
        const std::string& vehicle_id = "",
        const std::string& start_date = "",
        const std::string& end_date = "");

private:
    DatabaseManager* db;

    CalculationRecord rowToRecord(MYSQL_ROW row);
};

#endif