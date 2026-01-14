#include "Vehicle.h"
#include "Database_Manager.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <iomanip>

Vehicle::Vehicle(DatabaseManager* db)
    : db(db), vehicle_id(""), model_name(""), massKg(0), dragCoef(0),
    frontalArea(0), tirePressureBar(2.4), engineRatedPower(0),
    efficiency(0), hasAC(false) {
}

Vehicle::Vehicle(std::string id, double mass, double cd, double area, double power)
    : vehicle_id(id), massKg(mass), dragCoef(cd), frontalArea(area),
    engineRatedPower(power), tirePressureBar(2.4), efficiency(0),
    hasAC(false), db(nullptr) {
}

Vehicle::~Vehicle() {
}

// check if the vehicle exist
bool Vehicle::vehicleExists(const std::string& id) {
    if (!db || !db->getConnection()) return false;

    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    const char* sql = "SELECT COUNT(*) FROM vehicles WHERE vehicle_id = ?";

    bool exists = false;
    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) == 0) {
        MYSQL_BIND bind[1];
        memset(bind, 0, sizeof(bind));
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (char*)id.c_str();
        bind[0].buffer_length = (unsigned long)id.length();

        mysql_stmt_bind_param(stmt, bind);
        mysql_stmt_execute(stmt);

        int count = 0;
        MYSQL_BIND result_bind[1];
        memset(result_bind, 0, sizeof(result_bind));
        result_bind[0].buffer_type = MYSQL_TYPE_LONG;
        result_bind[0].buffer = &count;

        mysql_stmt_bind_result(stmt, result_bind);
        if (mysql_stmt_fetch(stmt) == 0) {
            exists = (count > 0);
        }
    }

    mysql_stmt_close(stmt);
    return exists;
}

// add vehicle to database
bool Vehicle::addVehicle(const std::string& id, const std::string& model, double eff,  double mass, double cd, double area, double power, double tirePressure, bool ac) {

    if (!db || !db->getConnection()) {
        std::cerr << "Database connection not available.\n";
        return false;
    }

    // Check if vehicle already exists
    if (vehicleExists(id)) {
        std::cout << "Vehicle ID '" << id << "' already exists. Use update instead.\n";
        return false;
    }

    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    const char* sql = "INSERT INTO vehicles (vehicle_id, model_name, base_efficiency, mass_kg, drag_coef, frontal_area, engine_rated_power, tire_pressure_bar, has_ac) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

    bool success = false;
    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) == 0) {
        MYSQL_BIND bind[9];
        memset(bind, 0, sizeof(bind));

        // Vehicle ID
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (char*)id.c_str();
        bind[0].buffer_length = (unsigned long)id.length();

        // Model Name
        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = (char*)model.c_str();
        bind[1].buffer_length = (unsigned long)model.length();

        // Base Efficiency
        bind[2].buffer_type = MYSQL_TYPE_DOUBLE;
        bind[2].buffer = (char*)&eff;

        // Mass (kg)
        bind[3].buffer_type = MYSQL_TYPE_DOUBLE;
        bind[3].buffer = (char*)&mass;

        // Drag Coefficient
        bind[4].buffer_type = MYSQL_TYPE_DOUBLE;
        bind[4].buffer = (char*)&cd;

        // Frontal Area
        bind[5].buffer_type = MYSQL_TYPE_DOUBLE;
        bind[5].buffer = (char*)&area;

        // Engine Rated Power
        bind[6].buffer_type = MYSQL_TYPE_DOUBLE;
        bind[6].buffer = (char*)&power;

        // Tire Pressure Bar
        bind[7].buffer_type = MYSQL_TYPE_DOUBLE;
        bind[7].buffer = (char*)&tirePressure;

        // Has AC (convert to int for MySQL)
        int acInt = ac ? 1 : 0;
        bind[8].buffer_type = MYSQL_TYPE_LONG;
        bind[8].buffer = (char*)&acInt;

        mysql_stmt_bind_param(stmt, bind);

        if (mysql_stmt_execute(stmt) == 0) {
            std::cout << "Vehicle '" << id << "' added successfully.\n";
            success = true;
        }
        else {
            std::cerr << "Failed to add vehicle: " << mysql_stmt_error(stmt) << std::endl;
        }
    }
    else {
        std::cerr << "Failed to prepare statement: " << mysql_stmt_error(stmt) << std::endl;
    }

    mysql_stmt_close(stmt);
    return success;
}

// update vehicle record in database
bool Vehicle::updateVehicle(const std::string& id, const std::string& model_name,
    double efficiency, double massKg, double dragCoef,
    double frontalArea, double engineRatedPower,
    double tirePressureBar, bool hasAC) {

    if (!db || !db->getConnection()) {
        std::cerr << "Database connection not available.\n";
        return false;
    }

    // First, check if vehicle exists using loadVehicle
    if (!loadVehicle(id)) {
        std::cout << "Vehicle with ID '" << id << "' does not exist. Cannot update.\n";
        return false;
    }

   
    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    if (!stmt) {
        std::cerr << "Failed to initialize update statement.\n";
        return false;
    }

    const char* update_sql = "UPDATE vehicles SET "
        "model_name = ?, "
        "base_efficiency = ?, "
        "mass_kg = ?, "
        "drag_coef = ?, "
        "frontal_area = ?, "
        "engine_rated_power = ?, "
        "tire_pressure_bar = ?, "
        "has_ac = ? "
        "WHERE vehicle_id = ?";

    if (mysql_stmt_prepare(stmt, update_sql, strlen(update_sql)) != 0) {
        std::cerr << "Failed to prepare update statement: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    // Prepare parameters
    MYSQL_BIND bind[9];
    memset(bind, 0, sizeof(bind));

    // Model name
    std::string model_name_str = model_name.empty() ? this->model_name : model_name;
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char*)model_name_str.c_str();
    bind[0].buffer_length = model_name_str.length();

    double final_efficiency = (efficiency <= 0) ? this->efficiency : efficiency;
    bind[1].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[1].buffer = &final_efficiency;

    // Mass
    double final_mass = (massKg <= 0) ? this->massKg : massKg;
    bind[2].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[2].buffer = &final_mass;

    // Drag coefficient
    double final_dragCoef = (dragCoef <= 0) ? this->dragCoef : dragCoef;
    bind[3].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[3].buffer = &final_dragCoef;

    // Frontal area
    double final_frontalArea = (frontalArea <= 0) ? this->frontalArea : frontalArea;
    bind[4].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[4].buffer = &final_frontalArea;

    // Engine power
    double final_enginePower = (engineRatedPower <= 0) ? this->engineRatedPower : engineRatedPower;
    bind[5].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[5].buffer = &final_enginePower;

    // Tire pressure
    double final_tirePressure = (tirePressureBar <= 0) ? this->tirePressureBar : tirePressureBar;
    bind[6].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[6].buffer = &final_tirePressure;

    // Has AC (convert bool to int)
    int has_ac_int = hasAC ? 1 : 0;
    bind[7].buffer_type = MYSQL_TYPE_LONG;
    bind[7].buffer = &has_ac_int;

    // Vehicle ID (WHERE clause)
    std::string vehicle_id = id;
    bind[8].buffer_type = MYSQL_TYPE_STRING;
    bind[8].buffer = (char*)vehicle_id.c_str();
    bind[8].buffer_length = vehicle_id.length();

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        std::cerr << "Failed to bind update parameters: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    // Execute the update
    if (mysql_stmt_execute(stmt) != 0) {
        std::cerr << "Failed to execute update: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    // Check if any rows were affected
    my_ulonglong affected_rows = mysql_stmt_affected_rows(stmt);

    mysql_stmt_close(stmt);

    if (affected_rows > 0) {
        std::cout << "Vehicle '" << id << "' updated successfully.\n";

        if (loadVehicle(id)) {
            return true;
        }
        else {
            std::cerr << "Warning: Vehicle updated but failed to reload.\n";
            return false;
        }
    }
    else {
        std::cout << "No changes made to vehicle '" << id << "' (values may be the same).\n";
        return true;
    }
}

// delete vehicle from database
bool Vehicle::deleteVehicle(const std::string& id) {
    if (!db || !db->getConnection()) {
        std::cerr << "Database connection not available.\n";
        return false;
    }

    // Check if vehicle exists
    if (!vehicleExists(id)) {
        std::cout << "Vehicle ID '" << id << "' does not exist.\n";
        return false;
    }

    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    // Fixed column name: vehicle_id
    const char* sql = "DELETE FROM vehicles WHERE vehicle_id = ?";

    bool success = false;
    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) == 0) {
        MYSQL_BIND bind[1];
        memset(bind, 0, sizeof(bind));
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (char*)id.c_str();
        bind[0].buffer_length = (unsigned long)id.length();

        mysql_stmt_bind_param(stmt, bind);

        if (mysql_stmt_execute(stmt) == 0) {
            if (mysql_stmt_affected_rows(stmt) > 0) {
                std::cout << "Vehicle '" << id << "' deleted successfully.\n";
                success = true;

                // Clear current object if it's the same vehicle
                if (vehicle_id == id) {
                    vehicle_id = "";
                    model_name = "";
                    massKg = 0;
                    dragCoef = 0;
                    frontalArea = 0;
                    engineRatedPower = 0;
                    efficiency = 0;
                    tirePressureBar = 2.4;
                    hasAC = false;
                }
            }
        }
        else {
            std::cerr << "Failed to delete vehicle: " << mysql_stmt_error(stmt) << std::endl;
        }
    }
    else {
        std::cerr << "Failed to prepare statement: " << mysql_stmt_error(stmt) << std::endl;
    }

    mysql_stmt_close(stmt);
    return success;
}

void Vehicle::listVehicles() {
    if (!db || !db->getConnection()) {
        std::cerr << "Database connection not available.\n";
        return;
    }

    std::string query = "SELECT vehicle_id, model_name, base_efficiency, mass_kg, drag_coef, frontal_area, "
        "engine_rated_power, tire_pressure_bar, has_ac FROM vehicles ORDER BY vehicle_id";

    if (mysql_query(db->getConnection(), query.c_str()) == 0) {
        MYSQL_RES* res = mysql_store_result(db->getConnection());
        if (res) {
            int num_fields = mysql_num_fields(res);
            MYSQL_ROW row;

            std::cout << "\n" << std::string(120, '-') << "\n";
            std::cout << std::left << std::setw(15) << "ID"
                << std::setw(20) << "Model"
                << std::setw(10) << "Mass(kg)"
                << std::setw(8) << "Cd"
                << std::setw(12) << "Area(m²)"
                << std::setw(10) << "Power(kW)"
                << std::setw(12) << "Tire(bar)"
                << std::setw(8) << "AC"
                << std::setw(12) << "Eff(km/L)"
                << "\n";
            std::cout << std::string(120, '-') << "\n";

            while ((row = mysql_fetch_row(res))) {
                std::cout << std::left << std::setw(15) << (row[0] ? row[0] : "")
                    << std::setw(20) << (row[1] ? row[1] : "")
                    << std::setw(10) << (row[3] ? row[3] : "0")  // mass_kg is column 3
                    << std::setw(8) << (row[4] ? row[4] : "0")   // drag_coef is column 4
                    << std::setw(12) << (row[5] ? row[5] : "0")  // frontal_area is column 5
                    << std::setw(10) << (row[6] ? row[6] : "0")  // engine_rated_power is column 6
                    << std::setw(12) << (row[7] ? row[7] : "2.4") // tire_pressure_bar is column 7
                    << std::setw(8) << (row[8] && std::atoi(row[8]) == 1 ? "Yes" : "No") // has_ac is column 8
                    << std::setw(12) << (row[2] ? row[2] : "0")  // base_efficiency is column 2
                    << "\n";
            }
            std::cout << std::string(120, '-') << "\n";
            std::cout << "Total vehicles: " << mysql_num_rows(res) << "\n";

            mysql_free_result(res);
        }
    }
    else {
        std::cerr << "Failed to list vehicles: " << mysql_error(db->getConnection()) << std::endl;
    }
}

bool Vehicle::loadVehicle(const std::string& id) {
    if (!db || !db->getConnection()) {
        std::cerr << "Database connection not available.\n";
        return false;
    }

    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    if (!stmt) {
        std::cerr << "Failed to initialize statement.\n";
        return false;
    }

    const char* sql = "SELECT vehicle_id, model_name, base_efficiency, mass_kg, drag_coef, frontal_area, engine_rated_power, tire_pressure_bar, has_ac FROM vehicles WHERE vehicle_id = ?";

    if (mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
        std::cerr << "Failed to prepare statement: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    // Bind the vehicle ID parameter
    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char*)id.c_str();
    bind[0].buffer_length = id.length();

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        std::cerr << "Failed to bind parameters: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        std::cerr << "Failed to execute query: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    // Bind result
    MYSQL_BIND result_bind[9];
    memset(result_bind, 0, sizeof(result_bind));

    char db_id[50];
    char model_name_buf[100];
    double efficiency, mass_kg, drag_coef, frontal_area, engine_power_kw, tire_pressure_bar;
    int has_ac;

    unsigned long length[9];
    my_bool is_null[9];

    // Vehicle ID
    result_bind[0].buffer_type = MYSQL_TYPE_STRING;
    result_bind[0].buffer = db_id;
    result_bind[0].buffer_length = sizeof(db_id);
    result_bind[0].length = &length[0];
    result_bind[0].is_null = &is_null[0];

    // Model name
    result_bind[1].buffer_type = MYSQL_TYPE_STRING;
    result_bind[1].buffer = model_name_buf;
    result_bind[1].buffer_length = sizeof(model_name_buf);
    result_bind[1].length = &length[1];
    result_bind[1].is_null = &is_null[1];

    // Efficiency
    result_bind[2].buffer_type = MYSQL_TYPE_DOUBLE;
    result_bind[2].buffer = &efficiency;
    result_bind[2].length = &length[2];
    result_bind[2].is_null = &is_null[2];

    // Mass
    result_bind[3].buffer_type = MYSQL_TYPE_DOUBLE;
    result_bind[3].buffer = &mass_kg;
    result_bind[3].length = &length[3];
    result_bind[3].is_null = &is_null[3];

    // Drag coefficient
    result_bind[4].buffer_type = MYSQL_TYPE_DOUBLE;
    result_bind[4].buffer = &drag_coef;
    result_bind[4].length = &length[4];
    result_bind[4].is_null = &is_null[4];

    // Frontal area
    result_bind[5].buffer_type = MYSQL_TYPE_DOUBLE;
    result_bind[5].buffer = &frontal_area;
    result_bind[5].length = &length[5];
    result_bind[5].is_null = &is_null[5];

    // Engine power
    result_bind[6].buffer_type = MYSQL_TYPE_DOUBLE;
    result_bind[6].buffer = &engine_power_kw;
    result_bind[6].length = &length[6];
    result_bind[6].is_null = &is_null[6];

    // Tire pressure
    result_bind[7].buffer_type = MYSQL_TYPE_DOUBLE;
    result_bind[7].buffer = &tire_pressure_bar;
    result_bind[7].length = &length[7];
    result_bind[7].is_null = &is_null[7];

    // Has AC
    result_bind[8].buffer_type = MYSQL_TYPE_LONG;
    result_bind[8].buffer = &has_ac;
    result_bind[8].length = &length[8];
    result_bind[8].is_null = &is_null[8];

    if (mysql_stmt_bind_result(stmt, result_bind) != 0) {
        std::cerr << "Failed to bind result: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    // Fetch the result
    if (mysql_stmt_fetch(stmt) == 0) {
        this->vehicle_id = db_id;
        this->model_name = model_name_buf;
        this->efficiency = efficiency;
        this->massKg = mass_kg;
        this->dragCoef = drag_coef;
        this->frontalArea = frontal_area;
        this->engineRatedPower = engine_power_kw;
        this->tirePressureBar = tire_pressure_bar;
        this->hasAC = (has_ac == 1);

        mysql_stmt_close(stmt);
        std::cout << "Vehicle '" << id << "' loaded successfully.\n";
        return true;
    }
    else {
        std::cout << "Vehicle with ID '" << id << "' not found in database.\n";
        mysql_stmt_close(stmt);
        return false;
    }
}

void Vehicle::displayVehicleDetails() const {
    std::cout << "\n=== Vehicle Details ===\n";
    std::cout << "ID: " << vehicle_id << "\n";
    std::cout << "Model: " << model_name << "\n";
    std::cout << "Mass: " << massKg << " kg\n";
    std::cout << "Drag Coefficient: " << dragCoef << "\n";
    std::cout << "Frontal Area: " << frontalArea << " m²\n";
    std::cout << "Engine Power: " << engineRatedPower << " kW\n";
    std::cout << "Tire Pressure: " << tirePressureBar << " bar\n";
    std::cout << "Air Conditioning: " << (hasAC ? "Yes" : "No") << "\n";
    std::cout << "Base Efficiency: " << efficiency << " km/L\n";
    std::cout << "========================\n";
}


//debug
/*
void Vehicle::debugCheckVehicle(const std::string& id) {
    if (!db || !db->getConnection()) {
        std::cerr << "[DEBUG] No database connection.\n";
        return;
    }

    std::cout << "[DEBUG] Checking for vehicle ID: '" << id << "'\n";
    std::cout << "[DEBUG] ID length: " << id.length() << "\n";
    std::cout << "[DEBUG] ID ascii values: ";
    for (char c : id) {
        std::cout << (int)c << " ";
    }
    std::cout << "\n";

    // Direct query to see what's in the database
    std::string query = "SELECT vehicle_id, model_name FROM vehicles WHERE id = '" + id + "'";
    std::cout << "[DEBUG] Query: " << query << "\n";

    if (mysql_query(db->getConnection(), query.c_str()) == 0) {
        MYSQL_RES* res = mysql_store_result(db->getConnection());
        if (res) {
            int num_rows = mysql_num_rows(res);
            std::cout << "[DEBUG] Found " << num_rows << " rows\n";

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                std::cout << "[DEBUG] Row: id='" << (row[0] ? row[0] : "NULL")
                    << "', model='" << (row[1] ? row[1] : "NULL") << "'\n";
            }
            mysql_free_result(res);
        }
    }
}
*/