#include <iostream>
#include <string>
#include <mysql.h> 
#include "Database_Manager.h"
#include "System.h"
#include "Auth.h"

int main() {
    system("cls");

    DatabaseManager db;
    if (!db.connect("localhost", "root", "", "fuel_efficiency", 3306)) {
        std::cerr << "Failed to connect to MySQL.\n";
        return 1;
    }

    Auth auth(&db);
    if (auth.registerUser("ghYz", "Password3140!")) {
        std::cout << "Demo user created successfully.\n";
    }



    System system(&db);

    std::string loggedInUser;

    if (auth.login(loggedInUser)) {
       while (system.run());
    }

    return 0;
}
