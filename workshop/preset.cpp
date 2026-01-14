#include "Preset.h"
#include <iostream>
#include <sstream>
#include <iomanip>

Preset::Preset(DatabaseManager* db) : db(db) {}

void Preset::savePreset(const std::string& name, double gradient, double roughness, double temperature) {
    std::stringstream ss;
    ss << "REPLACE INTO presets (name, road_gradient, surface_roughness, ambient_temp) VALUES ('"
        << name << "', " << gradient << ", " << roughness << ", " << temperature << ")";

    if (mysql_query(db->getConnection(), ss.str().c_str()) == 0) {
        std::cout << "Mission Profile '" << name << "' saved successfully.\n";
    }
    else {
        std::cerr << "Failed to save profile: " << mysql_error(db->getConnection()) << std::endl;
    }
}

bool Preset::loadPreset(const std::string& name, double& gradOut, double& roughOut, double& tempOut) {
    std::string query = "SELECT road_gradient, surface_roughness, ambient_temp FROM presets WHERE name='" + name + "'";

    if (mysql_query(db->getConnection(), query.c_str()) == 0) {
        MYSQL_RES* res = mysql_store_result(db->getConnection());
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row) {
            gradOut = std::stod(row[0]);
            roughOut = std::stod(row[1]);
            tempOut = std::stod(row[2]);
            mysql_free_result(res);
            return true;
        }
        mysql_free_result(res);
    }
    return false;
}

void Preset::listPresets() {
    std::cout << "\n--- Available Mission Profiles ---\n";
    if (mysql_query(db->getConnection(), "SELECT name, road_gradient, surface_roughness, ambient_temp FROM presets") == 0) {
        MYSQL_RES* res = mysql_store_result(db->getConnection());
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            std::cout << "Profile: " << std::left << std::setw(15) << row[0]
                << " | Grad: " << row[1]
                << " | Rough: " << row[2]
                << " | Temp: " << row[3] << "C" << std::endl;
        }
        mysql_free_result(res);
    }
}

bool Preset::deletePreset(const std::string& name) {
    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    const char* sql = "DELETE FROM presets WHERE name = ?";

    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) != 0) {
        std::cerr << "Failed to prepare delete statement: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char*)name.c_str();
    bind[0].buffer_length = (unsigned long)name.length();

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        std::cerr << "Failed to bind parameters: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        std::cerr << "Failed to execute delete: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    int affected_rows = (int)mysql_stmt_affected_rows(stmt);
    mysql_stmt_close(stmt);

    if (affected_rows > 0) {
        std::cout << "Preset '" << name << "' deleted successfully.\n";
        return true;
    }
    else {
        std::cout << "Preset '" << name << "' not found.\n";
        return false;
    }
}