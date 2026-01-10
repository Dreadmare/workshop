#include "Auth.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <random>

Auth::Auth(DatabaseManager* db) : db(db) {}

// Constant-time comparison to prevent timing attacks
bool Auth::security_compare(const std::string& a, const std::string& b) {
    if (a.length() != b.length()) return false;
    unsigned char result = 0;
    for (size_t i = 0; i < a.length(); ++i) {
        result |= (static_cast<unsigned char>(a[i]) ^ static_cast<unsigned char>(b[i]));
    }
    return result == 0;
}

std::string Auth::kdfHash(const std::string& password, const std::string& salt, int iterations) {
    std::string hash = password + salt;
    for (int i = 0; i < iterations; ++i) {
        hash += std::to_string(i);
    }
    return hash.substr(0, 64);
}

bool Auth::verify(const std::string& username, const std::string& password) {
    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    const char* query = "SELECT password_hash, salt FROM users WHERE username = ? LIMIT 1";

    if (mysql_stmt_prepare(stmt, query, strlen(query)) != 0) return false;

    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char*)username.c_str();
    bind[0].buffer_length = username.length();

    mysql_stmt_bind_param(stmt, bind);
    mysql_stmt_execute(stmt);

    // Bind results
    char h_buf[65], s_buf[33];
    unsigned long h_len, s_len;
    MYSQL_BIND res_bind[2];
    memset(res_bind, 0, sizeof(res_bind));

    res_bind[0].buffer_type = MYSQL_TYPE_STRING;
    res_bind[0].buffer = h_buf;
    res_bind[0].buffer_length = sizeof(h_buf);
    res_bind[0].length = &h_len;

    res_bind[1].buffer_type = MYSQL_TYPE_STRING;
    res_bind[1].buffer = s_buf;
    res_bind[1].buffer_length = sizeof(s_buf);
    res_bind[1].length = &s_len;

    mysql_stmt_bind_result(stmt, res_bind);

    bool authenticated = false;
    if (mysql_stmt_fetch(stmt) == 0) {
        std::string storedHash(h_buf, h_len);
        std::string salt(s_buf, s_len);

        std::string candidateHash = kdfHash(password, salt, 100000);
        if (security_compare(candidateHash, storedHash)) {
            authenticated = true;
        }
    }

    mysql_stmt_close(stmt);
    return authenticated;
}
void setEcho(bool enable) {
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdin, &mode);

    if (!enable) {
        mode &= ~ENABLE_ECHO_INPUT; // Turn off echo
    }
    else {
        mode |= ENABLE_ECHO_INPUT;  // Turn on echo
    }

    SetConsoleMode(hStdin, mode);
}

bool Auth::login(std::string& loggedInUser) {
    std::string u, p;
    std::cout << "--- Tactical Login ---\nUsername: ";
    std::cin >> u;
    std::cout << "Password: ";

    setEcho(false);
    std::cin >> p;
    setEcho(true);

    if (verify(u, p)) {
        loggedInUser = u;
        std::cout << "Access Granted. Welcome, " << u << ".\n";
        return true;
    }
    else {
        std::cout << "Access Denied.\n";
        return false;
    }
}

bool Auth::passwordPolicy(const std::string& pw) {
    if (pw.length() < 12) return false;
    bool hasUpper = false, hasLower = false, hasDigit = false;
    for (char c : pw) {
        if (isupper(c)) hasUpper = true;
        if (islower(c)) hasLower = true;
        if (isdigit(c)) hasDigit = true;
    }
    return hasUpper && hasLower && hasDigit;
}

std::string Auth::generateSalt(std::size_t length) {
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string salt;
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);

    for (std::size_t i = 0; i < length; ++i) {
        salt += charset[dist(generator)];
    }
    return salt;
}

bool Auth::registerUser(const std::string& username, const std::string& password, Role role) {
    // Validate Password Strength
    if (!passwordPolicy(password)) {
        std::cout << "Registration failed: Password does not meet security policy (min 12 chars, upper, lower, digit).\n";
        return false;
    }
    // Unique salt
    std::string salt = generateSalt(16);

    // Has
    std::string passwordHash = kdfHash(password, salt, 100000);

    // Insert into DB with Prepared Statements to prevent SQL injections
    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    const char* sql = "INSERT INTO users (username, password_hash, salt, role) VALUES (?, ?, ?, ?)";

    if (mysql_stmt_prepare(stmt, sql, strlen(sql)) == 0) {
        MYSQL_BIND bind[4];
        memset(bind, 0, sizeof(bind));

        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (char*)username.c_str();
        bind[0].buffer_length = username.length();

        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = (char*)passwordHash.c_str();
        bind[1].buffer_length = passwordHash.length();

        bind[2].buffer_type = MYSQL_TYPE_STRING;
        bind[2].buffer = (char*)salt.c_str();
        bind[2].buffer_length = salt.length();

        // 1 = admin, 0 = user
        int roleInt = (role == Role::ADMIN) ? 1 : 0;
        bind[3].buffer_type = MYSQL_TYPE_LONG;
        bind[3].buffer = (char*)&roleInt;

        mysql_stmt_bind_param(stmt, bind);

        if (mysql_stmt_execute(stmt) == 0) {
            std::cout << "User '" << username << "' registered successfully.\n";
            mysql_stmt_close(stmt);
            return true;
        }
        else {
            std::cerr << "Registration Error: " << mysql_stmt_error(stmt) << "\n";
        }
    }

    mysql_stmt_close(stmt);
    return false;
}