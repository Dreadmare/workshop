#include "Preset.h"
#include <iostream>

Preset::Preset(DatabaseManager* db) : db(db) {}

void Preset::savePreset(const std::string& id,const std::string& name, const std::string& terrain, const std::string& climate) {
    std::string query = "INSERT INTO presets (name, terrain, climate) VALUES('"+ id +"," +
        name + "','" + terrain + "','" + climate + "')";
    if (mysql_query(db->getConnection(), query.c_str()) == 0) {
        std::cout << "Preset " << name << " saved successfully.\n";
    }
    else {
        std::cerr << "Failed to save preset: " << mysql_error(db->getConnection()) << std::endl;
    }
}

bool Preset::loadPreset(const std::string& name, std::string& terrainOut, std::string& climateOut) {
    std::string query = "SELECT terrain, climate FROM presets WHERE name='" + name + "'";
    if (mysql_query(db->getConnection(), query.c_str()) == 0) {
        MYSQL_RES* res = mysql_store_result(db->getConnection());
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row) {
            terrainOut = row[0];
            climateOut = row[1];
            mysql_free_result(res);
            return true;
        }
        mysql_free_result(res);
    }
    else {
        std::cerr << "Query failed: " << mysql_error(db->getConnection()) << std::endl;
    }
    return false;
}

void Preset::listPresets() {
    if (mysql_query(db->getConnection(), "SELECT * FROM presets") == 0) {
        MYSQL_RES* res = mysql_store_result(db->getConnection());
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            std::cout << "Preset: " << row[0] << " | Terrain: " << row[1]
                << " | Climate: " << row[2] << std::endl;
        }
        mysql_free_result(res);
    }
    else {
        std::cerr << "Query failed: " << mysql_error(db->getConnection()) << std::endl;
    }
}
