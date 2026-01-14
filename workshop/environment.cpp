#include "Environment.h"
#include "Database_Manager.h"
#include <mysql.h>
#include <cstring>
#include <iostream>

// default
void Environment::setRawEnvironment(double grad, double rough, double temp) {
    this->roadGradient = grad;
    this->surfaceRoughness = rough;
    this->ambientTempC = temp;
    this->pressurePa = 101325.0;
}

double Environment::getAirDensity() const {
    const double R_specific = 287.058;
    double T_kelvin = this->ambientTempC + 273.15;
    return this->pressurePa / (R_specific * T_kelvin);
}

void Environment::loadEnvironment(const std::string& tType, const std::string& cType, DatabaseManager* db) {
    MYSQL* conn = db->getConnection();
    if (!conn) return;
    std::string query = "SELECT gradient, roughness, temperature, pressure FROM environment_presets WHERE terrain_type = ? OR climate_type = ? LIMIT 1";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return;

    if (mysql_stmt_prepare(stmt, query.c_str(), (unsigned long)query.length()) == 0) {
        MYSQL_BIND bind[2];
        memset(bind, 0, sizeof(bind));

        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (char*)tType.c_str();
        bind[0].buffer_length = (unsigned long)tType.length();

        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = (char*)cType.c_str();
        bind[1].buffer_length = (unsigned long)cType.length();

        mysql_stmt_bind_param(stmt, bind);

        if (mysql_stmt_execute(stmt) == 0) {
            double grad, rough, temp, press;
            unsigned long len;
            my_bool is_null;

            MYSQL_BIND res_bind[4];
            memset(res_bind, 0, sizeof(res_bind));

            res_bind[0].buffer_type = MYSQL_TYPE_DOUBLE;
            res_bind[0].buffer = &grad;

            res_bind[1].buffer_type = MYSQL_TYPE_DOUBLE;
            res_bind[1].buffer = &rough;

            res_bind[2].buffer_type = MYSQL_TYPE_DOUBLE;
            res_bind[2].buffer = &temp;

            res_bind[3].buffer_type = MYSQL_TYPE_DOUBLE;
            res_bind[3].buffer = &press;

            mysql_stmt_bind_result(stmt, res_bind);

            if (mysql_stmt_fetch(stmt) == 0) {
                this->roadGradient = grad;
                this->surfaceRoughness = rough;
                this->ambientTempC = temp;
                this->pressurePa = press;
                std::cout << "[DB] Environment loaded from presets.\n";
            }
        }
    }
    mysql_stmt_close(stmt);
}