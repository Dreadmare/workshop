#include "Calculation_History.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <ctime>
#include <cstring>
#include <mysql.h>
#include <algorithm>

CalculationHistory::CalculationHistory(DatabaseManager* db) : db(db) {}

std::string CalculationRecord::getFormattedDate() const {
    if (calculated_at.empty()) return "Unknown";

    std::tm tm = {};
    std::istringstream ss(calculated_at);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

    if (ss.fail()) {
   
        return calculated_at;
    }

    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &tm);
    return std::string(buffer);
}

double CalculationRecord::getTotalFuelCost() const {
    return fuel_consumed_liters * 2.0;
}

bool CalculationHistory::saveCalculation(const CalculationRecord& record) {
    if (!db || !db->getConnection()) {
        std::cerr << "Database connection not available.\n";
        return false;
    }

    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    const char* sql = "INSERT INTO calculation_history (username, vehicle_id, mission_name, "
        "vehicle_mass, vehicle_drag_coef, vehicle_frontal_area, vehicle_tire_pressure, "
        "vehicle_engine_power, vehicle_has_ac, vehicle_efficiency, road_gradient, "
        "surface_roughness, ambient_temp, pressure, distance_km, avg_speed_kmh, "
        "fuel_consumed_liters, cost_per_km) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) != 0) {
        std::cerr << "Failed to prepare save statement: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND bind[18];
    memset(bind, 0, sizeof(bind));

    // Bind parameters
    // 0: username
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char*)record.username.c_str();
    bind[0].buffer_length = (unsigned long)record.username.length();

    // 1: vehicle_id
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (char*)record.vehicle_id.c_str();
    bind[1].buffer_length = (unsigned long)record.vehicle_id.length();

    // 2: mission_name
    bind[2].buffer_type = MYSQL_TYPE_STRING;
    bind[2].buffer = (char*)record.mission_name.c_str();
    bind[2].buffer_length = (unsigned long)record.mission_name.length();

    // 3: vehicle_mass (double)
    bind[3].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[3].buffer = (char*)&record.vehicle_mass;

    // 4: vehicle_drag_coef (double)
    bind[4].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[4].buffer = (char*)&record.vehicle_drag_coef;

    // 5: vehicle_frontal_area (double)
    bind[5].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[5].buffer = (char*)&record.vehicle_frontal_area;

    // 6: vehicle_tire_pressure (double)
    bind[6].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[6].buffer = (char*)&record.vehicle_tire_pressure;

    // 7: vehicle_engine_power (double)
    bind[7].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[7].buffer = (char*)&record.vehicle_engine_power;

    // 8: vehicle_has_ac (int from bool)
    int has_ac_int = record.vehicle_has_ac ? 1 : 0;
    bind[8].buffer_type = MYSQL_TYPE_LONG;
    bind[8].buffer = (char*)&has_ac_int;

    // 9: vehicle_efficiency (double)
    bind[9].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[9].buffer = (char*)&record.vehicle_efficiency;

    // 10: road_gradient (double)
    bind[10].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[10].buffer = (char*)&record.road_gradient;

    // 11: surface_roughness (double)
    bind[11].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[11].buffer = (char*)&record.surface_roughness;

    // 12: ambient_temp (double)
    bind[12].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[12].buffer = (char*)&record.ambient_temp;

    // 13: pressure (double)
    bind[13].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[13].buffer = (char*)&record.pressure;

    // 14: distance_km (double)
    bind[14].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[14].buffer = (char*)&record.distance_km;

    // 15: avg_speed_kmh (double)
    bind[15].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[15].buffer = (char*)&record.avg_speed_kmh;

    // 16: fuel_consumed_liters (double)
    bind[16].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[16].buffer = (char*)&record.fuel_consumed_liters;

    // 17: cost_per_km (double)
    bind[17].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[17].buffer = (char*)&record.cost_per_km;

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        std::cerr << "Failed to bind parameters: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    bool success = (mysql_stmt_execute(stmt) == 0);

    if (success) {
        std::cout << "Calculation saved to history.\n";
    }
    else {
        std::cerr << "Failed to save calculation: " << mysql_stmt_error(stmt) << std::endl;
    }

    mysql_stmt_close(stmt);
    return success;
}

std::vector<CalculationRecord> CalculationHistory::getUserCalculations(const std::string& username, int limit) {
    std::vector<CalculationRecord> records;

    if (!db || !db->getConnection()) {
        return records;
    }

    std::string query = "SELECT * FROM calculation_history WHERE username = ? "
        "ORDER BY calculated_at DESC LIMIT ?";

    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    if (!stmt) {
        return records;
    }

    if (mysql_stmt_prepare(stmt, query.c_str(), (unsigned long)query.length()) != 0) {
        std::cerr << "Failed to prepare statement: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return records;
    }

    // Bind parameters
    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));

    // Username parameter
    std::string user_param = username;
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char*)user_param.c_str();
    bind[0].buffer_length = (unsigned long)user_param.length();

    // Limit parameter
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = (char*)&limit;

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        std::cerr << "Failed to bind parameters: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return records;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        std::cerr << "Failed to execute query: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return records;
    }

    // Simple approach: use regular query instead of prepared statement for results
    mysql_stmt_close(stmt);

    // Use direct query for simplicity
    std::string direct_query = "SELECT * FROM calculation_history WHERE username = '" +
        username + "' ORDER BY calculated_at DESC LIMIT " +
        std::to_string(limit);

    if (mysql_query(db->getConnection(), direct_query.c_str()) == 0) {
        MYSQL_RES* res = mysql_store_result(db->getConnection());
        if (res) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                records.push_back(rowToRecord(row));
            }
            mysql_free_result(res);
        }
    }

    return records;
}

std::vector<CalculationRecord> CalculationHistory::getRecentCalculations(int limit) {
    std::vector<CalculationRecord> records;

    if (!db || !db->getConnection()) {
        return records;
    }

    std::string query = "SELECT * FROM calculation_history ORDER BY calculated_at DESC LIMIT " +
        std::to_string(limit);

    if (mysql_query(db->getConnection(), query.c_str()) == 0) {
        MYSQL_RES* res = mysql_store_result(db->getConnection());
        if (res) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                records.push_back(rowToRecord(row));
            }
            mysql_free_result(res);
        }
    }

    return records;
}

std::vector<CalculationRecord> CalculationHistory::getVehicleCalculations(const std::string& vehicle_id, int limit) {
    std::vector<CalculationRecord> records;

    if (!db || !db->getConnection()) {
        return records;
    }

    std::string query = "SELECT * FROM calculation_history WHERE vehicle_id = ? "
        "ORDER BY calculated_at DESC LIMIT ?";

    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    if (!stmt) {
        return records;
    }

    if (mysql_stmt_prepare(stmt, query.c_str(), (unsigned long)query.length()) != 0) {
        std::cerr << "Failed to prepare statement: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return records;
    }

    // Bind parameters
    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));

    // Vehicle ID parameter
    std::string vehicle_param = vehicle_id;
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char*)vehicle_param.c_str();
    bind[0].buffer_length = (unsigned long)vehicle_param.length();

    // Limit parameter
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = (char*)&limit;

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        std::cerr << "Failed to bind parameters: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return records;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        std::cerr << "Failed to execute query: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return records;
    }

    mysql_stmt_close(stmt);

    // Use direct query for results
    std::string direct_query = "SELECT * FROM calculation_history WHERE vehicle_id = '" +
        vehicle_id + "' ORDER BY calculated_at DESC LIMIT " +
        std::to_string(limit);

    if (mysql_query(db->getConnection(), direct_query.c_str()) == 0) {
        MYSQL_RES* res = mysql_store_result(db->getConnection());
        if (res) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                records.push_back(rowToRecord(row));
            }
            mysql_free_result(res);
        }
    }

    return records;
}

CalculationRecord CalculationHistory::getCalculationById(int calculation_id) {
    CalculationRecord record;

    if (!db || !db->getConnection()) {
        return record;
    }

    std::string query = "SELECT * FROM calculation_history WHERE id = " +
        std::to_string(calculation_id);

    if (mysql_query(db->getConnection(), query.c_str()) == 0) {
        MYSQL_RES* res = mysql_store_result(db->getConnection());
        if (res && mysql_num_rows(res) > 0) {
            MYSQL_ROW row = mysql_fetch_row(res);
            record = rowToRecord(row);
        }
        mysql_free_result(res);
    }

    return record;
}

bool CalculationHistory::deleteCalculation(int calculation_id, const std::string& requesting_user) {
    if (!db || !db->getConnection()) {
        std::cerr << "Database connection not available.\n";
        return false;
    }

    // First check if calculation exists and user has permission
    CalculationRecord record = getCalculationById(calculation_id);
    if (record.username.empty()) {
        std::cout << "Calculation not found.\n";
        return false;
    }

    if (record.username != requesting_user) {
        std::cout << "Access denied: You can only delete your own calculations.\n";
        return false;
    }

    std::string query = "DELETE FROM calculation_history WHERE id = " +
        std::to_string(calculation_id);

    if (mysql_query(db->getConnection(), query.c_str()) == 0) {
        if (mysql_affected_rows(db->getConnection()) > 0) {
            std::cout << "Calculation deleted successfully.\n";
            return true;
        }
    }

    std::cerr << "Failed to delete calculation.\n";
    return false;
}

bool CalculationHistory::deleteAllUserCalculations(const std::string& username) {
    if (!db || !db->getConnection()) {
        std::cerr << "Database connection not available.\n";
        return false;
    }

    std::string query = "DELETE FROM calculation_history WHERE username = ?";

    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    if (!stmt) {
        return false;
    }

    if (mysql_stmt_prepare(stmt, query.c_str(), (unsigned long)query.length()) != 0) {
        std::cerr << "Failed to prepare statement: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    // Bind parameter
    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));

    std::string user_param = username;
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char*)user_param.c_str();
    bind[0].buffer_length = (unsigned long)user_param.length();

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        std::cerr << "Failed to bind parameters: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    bool success = (mysql_stmt_execute(stmt) == 0);

    if (success) {
        std::cout << "All calculations for user '" << username << "' deleted successfully.\n";
    }
    else {
        std::cerr << "Failed to delete calculations: " << mysql_stmt_error(stmt) << std::endl;
    }

    mysql_stmt_close(stmt);
    return success;
}

int CalculationHistory::getCalculationCount(const std::string& username) {
    if (!db || !db->getConnection()) {
        return 0;
    }

    std::string query;
    if (username.empty()) {
        query = "SELECT COUNT(*) FROM calculation_history";
    }
    else {
        query = "SELECT COUNT(*) FROM calculation_history WHERE username = ?";
    }

    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    if (!stmt) {
        return 0;
    }

    if (mysql_stmt_prepare(stmt, query.c_str(), (unsigned long)query.length()) != 0) {
        std::cerr << "Failed to prepare statement: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return 0;
    }

    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));

    if (!username.empty()) {
        std::string user_param = username;
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (char*)user_param.c_str();
        bind[0].buffer_length = (unsigned long)user_param.length();
    }

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        std::cerr << "Failed to bind parameters: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return 0;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        std::cerr << "Failed to execute query: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return 0;
    }

    // Bind result
    MYSQL_BIND result_bind[1];
    memset(result_bind, 0, sizeof(result_bind));

    int count = 0;
    result_bind[0].buffer_type = MYSQL_TYPE_LONG;
    result_bind[0].buffer = &count;

    if (mysql_stmt_bind_result(stmt, result_bind) != 0) {
        std::cerr << "Failed to bind result: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return 0;
    }

    if (mysql_stmt_fetch(stmt) != 0) {
        std::cerr << "Failed to fetch result: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return 0;
    }

    mysql_stmt_close(stmt);
    return count;
}

double CalculationHistory::getTotalFuelConsumed(const std::string& username) {
    if (!db || !db->getConnection()) {
        return 0.0;
    }

    std::string query;
    if (username.empty()) {
        query = "SELECT SUM(fuel_consumed_liters) FROM calculation_history";
    }
    else {
        query = "SELECT SUM(fuel_consumed_liters) FROM calculation_history WHERE username = ?";
    }

    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    if (!stmt) {
        return 0.0;
    }

    if (mysql_stmt_prepare(stmt, query.c_str(), (unsigned long)query.length()) != 0) {
        std::cerr << "Failed to prepare statement: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return 0.0;
    }

    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));

    if (!username.empty()) {
        std::string user_param = username;
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (char*)user_param.c_str();
        bind[0].buffer_length = (unsigned long)user_param.length();
    }

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        std::cerr << "Failed to bind parameters: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return 0.0;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        std::cerr << "Failed to execute query: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return 0.0;
    }

    // Bind result
    MYSQL_BIND result_bind[1];
    memset(result_bind, 0, sizeof(result_bind));

    double total_fuel = 0.0;
    result_bind[0].buffer_type = MYSQL_TYPE_DOUBLE;
    result_bind[0].buffer = &total_fuel;

    if (mysql_stmt_bind_result(stmt, result_bind) != 0) {
        std::cerr << "Failed to bind result: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return 0.0;
    }

    if (mysql_stmt_fetch(stmt) != 0) {
        mysql_stmt_close(stmt);
        return 0.0;
    }

    mysql_stmt_close(stmt);
    return total_fuel;
}

double CalculationHistory::getAverageFuelConsumption(const std::string& username) {
    if (!db || !db->getConnection()) {
        return 0.0;
    }

    std::string query;
    if (username.empty()) {
        query = "SELECT AVG(fuel_consumed_liters) FROM calculation_history";
    }
    else {
        query = "SELECT AVG(fuel_consumed_liters) FROM calculation_history WHERE username = ?";
    }

    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    if (!stmt) {
        return 0.0;
    }

    if (mysql_stmt_prepare(stmt, query.c_str(), (unsigned long)query.length()) != 0) {
        std::cerr << "Failed to prepare statement: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return 0.0;
    }

    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));

    if (!username.empty()) {
        std::string user_param = username;
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (char*)user_param.c_str();
        bind[0].buffer_length = (unsigned long)user_param.length();
    }

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        std::cerr << "Failed to bind parameters: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return 0.0;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        std::cerr << "Failed to execute query: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return 0.0;
    }

    // Bind result
    MYSQL_BIND result_bind[1];
    memset(result_bind, 0, sizeof(result_bind));

    double avg_fuel = 0.0;
    result_bind[0].buffer_type = MYSQL_TYPE_DOUBLE;
    result_bind[0].buffer = &avg_fuel;

    if (mysql_stmt_bind_result(stmt, result_bind) != 0) {
        std::cerr << "Failed to bind result: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return 0.0;
    }

    if (mysql_stmt_fetch(stmt) != 0) {
        mysql_stmt_close(stmt);
        return 0.0;
    }

    mysql_stmt_close(stmt);
    return avg_fuel;
}

bool CalculationHistory::exportToCSV(const std::string& username, const std::string& filename) {
    std::vector<CalculationRecord> records;

    if (username.empty() || username == "ALL") {
        records = getRecentCalculations(1000); // Export all recent records
    }
    else {
        records = getUserCalculations(username, 1000);
    }

    if (records.empty()) {
        std::cout << "No calculations to export.\n";
        return false;
    }

    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << "\n";
        return false;
    }

    // Write CSV header
    file << "ID,Username,VehicleID,MissionName,Date,Distance(km),AvgSpeed(km/h),"
        << "FuelConsumed(L),CostPerKm,RoadGradient,SurfaceRoughness,AmbientTemp(C),"
        << "VehicleMass(kg),DragCoefficient,FrontalArea(m2),EnginePower(kW),"
        << "TirePressure(bar),HasAC,VehicleEfficiency(km/L)\n";

    // Write data rows
    for (const auto& record : records) {
        file << record.id << ","
            << "\"" << record.username << "\","
            << "\"" << record.vehicle_id << "\","
            << "\"" << record.mission_name << "\","
            << "\"" << record.getFormattedDate() << "\","
            << record.distance_km << ","
            << record.avg_speed_kmh << ","
            << record.fuel_consumed_liters << ","
            << record.cost_per_km << ","
            << record.road_gradient << ","
            << record.surface_roughness << ","
            << record.ambient_temp << ","
            << record.vehicle_mass << ","
            << record.vehicle_drag_coef << ","
            << record.vehicle_frontal_area << ","
            << record.vehicle_engine_power << ","
            << record.vehicle_tire_pressure << ","
            << (record.vehicle_has_ac ? "Yes" : "No") << ","
            << record.vehicle_efficiency << "\n";
    }

    file.close();
    std::cout << "Exported " << records.size() << " records to " << filename << "\n";
    return true;
}

std::vector<CalculationRecord> CalculationHistory::searchCalculations(
    const std::string& username,
    const std::string& vehicle_id,
    const std::string& start_date,
    const std::string& end_date) {

    std::vector<CalculationRecord> records;

    if (!db || !db->getConnection()) {
        return records;
    }

    // Build dynamic query
    std::string query = "SELECT * FROM calculation_history WHERE 1=1";
    std::vector<std::string> params;

    if (!username.empty()) {
        query += " AND username = ?";
        params.push_back(username);
    }

    if (!vehicle_id.empty()) {
        query += " AND vehicle_id = ?";
        params.push_back(vehicle_id);
    }

    if (!start_date.empty()) {
        query += " AND calculated_at >= ?";
        params.push_back(start_date);
    }

    if (!end_date.empty()) {
        query += " AND calculated_at <= ?";
        params.push_back(end_date + " 23:59:59");
    }

    query += " ORDER BY calculated_at DESC LIMIT 100";

    // Use prepared statement
    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    if (!stmt) {
        return records;
    }

    if (mysql_stmt_prepare(stmt, query.c_str(), (unsigned long)query.length()) != 0) {
        std::cerr << "Failed to prepare statement: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return records;
    }

    // Bind parameters if any
    if (!params.empty()) {
        MYSQL_BIND* bind = new MYSQL_BIND[params.size()];
        memset(bind, 0, sizeof(MYSQL_BIND) * params.size());

        for (size_t i = 0; i < params.size(); i++) {
            bind[i].buffer_type = MYSQL_TYPE_STRING;
            bind[i].buffer = (char*)params[i].c_str();
            bind[i].buffer_length = (unsigned long)params[i].length();
        }

        if (mysql_stmt_bind_param(stmt, bind) != 0) {
            std::cerr << "Failed to bind parameters: " << mysql_stmt_error(stmt) << std::endl;
            delete[] bind;
            mysql_stmt_close(stmt);
            return records;
        }

        delete[] bind;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        std::cerr << "Failed to execute query: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return records;
    }

    mysql_stmt_close(stmt);

    // Use direct query for simplicity in fetching results
    std::string direct_query = "SELECT * FROM calculation_history WHERE 1=1";

    if (!username.empty()) {
        direct_query += " AND username = '" + username + "'";
    }

    if (!vehicle_id.empty()) {
        direct_query += " AND vehicle_id = '" + vehicle_id + "'";
    }

    if (!start_date.empty()) {
        direct_query += " AND calculated_at >= '" + start_date + "'";
    }

    if (!end_date.empty()) {
        direct_query += " AND calculated_at <= '" + end_date + " 23:59:59'";
    }

    direct_query += " ORDER BY calculated_at DESC LIMIT 100";

    if (mysql_query(db->getConnection(), direct_query.c_str()) == 0) {
        MYSQL_RES* res = mysql_store_result(db->getConnection());
        if (res) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                records.push_back(rowToRecord(row));
            }
            mysql_free_result(res);
        }
    }

    return records;
}

CalculationRecord CalculationHistory::rowToRecord(MYSQL_ROW row) {
    CalculationRecord record;

    if (row[0]) record.id = std::stoi(row[0]);  // id
    if (row[1]) record.username = row[1];  // username
    if (row[2]) record.vehicle_id = row[2];  // vehicle_id
    if (row[3]) record.mission_name = row[3];  // mission_name

    if (row[4]) record.vehicle_mass = std::stod(row[4]);  // vehicle_mass
    if (row[5]) record.vehicle_drag_coef = std::stod(row[5]);  // vehicle_drag_coef
    if (row[6]) record.vehicle_frontal_area = std::stod(row[6]);  // vehicle_frontal_area
    if (row[7]) record.vehicle_tire_pressure = std::stod(row[7]);  // vehicle_tire_pressure
    if (row[8]) record.vehicle_engine_power = std::stod(row[8]);  // vehicle_engine_power
    if (row[9]) record.vehicle_has_ac = (std::stoi(row[9]) == 1);  // vehicle_has_ac
    if (row[10]) record.vehicle_efficiency = std::stod(row[10]);  // vehicle_efficiency

    if (row[11]) record.road_gradient = std::stod(row[11]);  // road_gradient
    if (row[12]) record.surface_roughness = std::stod(row[12]);  // surface_roughness
    if (row[13]) record.ambient_temp = std::stod(row[13]);  // ambient_temp
    if (row[14]) record.pressure = std::stod(row[14]);  // pressure

    if (row[15]) record.distance_km = std::stod(row[15]);  // distance_km
    if (row[16]) record.avg_speed_kmh = std::stod(row[16]);  // avg_speed_kmh

    if (row[17]) record.fuel_consumed_liters = std::stod(row[17]);  // fuel_consumed_liters
    if (row[18]) record.cost_per_km = std::stod(row[18]);  // cost_per_km

    if (row[19]) record.calculated_at = row[19];  // calculated_at

    return record;
}