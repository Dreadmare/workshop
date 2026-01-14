#include <iostream>
#include <string>
#include <mysql.h> 
#include "Database_Manager.h"
#include "System.h"
#include "Auth.h"

int main() {
    system("cls");

    std::cout << "========================================\n";
    std::cout << "  FUEL MANAGEMENT SYSTEM\n";
    std::cout << "========================================\n\n";

    // Connect to database
    DatabaseManager db;
    if (!db.connect("localhost", "root", "", "fuel_efficiency", 3306)) {
        std::cerr << "Failed to connect to MySQL.\n";
        std::cout << "Press Enter to exit...";
        std::cin.get();
        return 1;
    }

    Auth auth(&db);

    // Check if any users exist
    bool hasUsers = false;
    MYSQL* conn = db.getConnection();
    if (conn) {
        if (mysql_query(conn, "SELECT COUNT(*) FROM users") == 0) {
            MYSQL_RES* res = mysql_store_result(conn);
            MYSQL_ROW row = mysql_fetch_row(res);
            int count = row ? std::atoi(row[0]) : 0;
            mysql_free_result(res);
            hasUsers = (count > 0);
        }
    }

    // default admin
    if (!hasUsers) {
        std::cout << "No users found in database.\n";
        std::cout << "Creating default administrator account...\n";

        if (auth.registerUser("admin", "admin123", Auth::Role::ADMIN)) {
            std::cout << "✓ Default admin created:\n";
            std::cout << "  Username: admin\n";
            std::cout << "  Password: admin123\n";
            std::cout << "  Role: admin\n";
            std::cout << "\nPlease change the password after first login!\n";
        }
    }

    std::cout << "\n========================================\n";

    // Start the system
    System system(&db);
    system.runApplication();

    std::cout << "\nThank you for using the Tactical Fuel Management System!\n";
    std::cout << "Press Enter to exit...";
    std::cin.ignore();
    std::cin.get();

    return 0;
}