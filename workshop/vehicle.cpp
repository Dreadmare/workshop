#include "Vehicle.h"
#include "Database_Manager.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <iomanip>

Vehicle::Vehicle(DatabaseManager* db)
    : db(db), id(""), massKg(0), dragCoef(0), frontalArea(0), tirePressureBar(0), engineRatedPower(0), hasAC(false) {
}

Vehicle::Vehicle(std::string id, double mass, double cd, double area, double power)
    : id(id), massKg(mass), dragCoef(cd), frontalArea(area),
    tirePressureBar(2.4), engineRatedPower(power), hasAC(true), db(nullptr) {
}

void Vehicle::addVehicle(const std::string& id, const std::string& model, double efficiency) {
    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    const char* sql = "INSERT INTO vehicles (id, model, efficiency) VALUES (?, ?, ?)";

    if (mysql_stmt_prepare(stmt, sql, strlen(sql)) == 0) {
        MYSQL_BIND bind[3];
        memset(bind, 0, sizeof(bind));

        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (char*)id.c_str();
        bind[0].buffer_length = id.length();

        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = (char*)model.c_str();
        bind[1].buffer_length = model.length();

        bind[2].buffer_type = MYSQL_TYPE_DOUBLE;
        bind[2].buffer = (char*)&efficiency;

        mysql_stmt_bind_param(stmt, bind);

        if (mysql_stmt_execute(stmt) == 0) {
            std::cout << "Vehicle [" << id << "] added successfully.\n";
        }
        else {
            std::cerr << "Execution Error: " << mysql_stmt_error(stmt) << "\n";
        }
    }
    else {
        std::cerr << "Preparation Error: " << mysql_stmt_error(stmt) << "\n";
    }

    mysql_stmt_close(stmt);
}

void Vehicle::listVehicles() {
    const char* query = "SELECT id, model, efficiency FROM vehicles";

    if (mysql_query(db->getConnection(), query) == 0) {
        MYSQL_RES* result = mysql_store_result(db->getConnection());
        if (result) {
            MYSQL_ROW row;
            std::cout << "\n--- Registered Vehicles ---\n";
            std::cout << std::left << std::setw(10) << "ID"
                << std::setw(20) << "Model"
                << "Efficiency (km/L)\n";
            std::cout << "-------------------------------------------\n";

            while ((row = mysql_fetch_row(result))) {
                std::cout << std::left << std::setw(10) << (row[0] ? row[0] : "N/A")
                    << std::setw(20) << (row[1] ? row[1] : "N/A")
                    << (row[2] ? row[2] : "0.0") << "\n";
            }
            mysql_free_result(result);
        }
    }
    else {
        std::cerr << "Failed to fetch vehicles: " << mysql_error(db->getConnection()) << "\n";
    }
}

double Vehicle::getVehicleEfficiency(const std::string& id) {
    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    const char* sql = "SELECT efficiency FROM vehicles WHERE id = ? LIMIT 1";
    double efficiency = 0.0;

    if (mysql_stmt_prepare(stmt, sql, strlen(sql)) == 0) {
        MYSQL_BIND b_in[1];
        memset(b_in, 0, sizeof(b_in));
        b_in[0].buffer_type = MYSQL_TYPE_STRING;
        b_in[0].buffer = (char*)id.c_str();
        b_in[0].buffer_length = id.length();

        mysql_stmt_bind_param(stmt, b_in);
        mysql_stmt_execute(stmt);

        MYSQL_BIND b_out[1];
        memset(b_out, 0, sizeof(b_out));
        b_out[0].buffer_type = MYSQL_TYPE_DOUBLE;
        b_out[0].buffer = (char*)&efficiency;

        mysql_stmt_bind_result(stmt, b_out);

        if (mysql_stmt_fetch(stmt) != 0) {
            efficiency = -1.0;
        }
    }
    mysql_stmt_close(stmt);
    return efficiency;
}