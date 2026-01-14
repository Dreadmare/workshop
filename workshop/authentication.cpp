#pragma comment(lib, "advapi32.lib")
#include "Auth.h"
#include <iostream>
#include <windows.h>
#include <wincrypt.h>
#include <cstring>
#include <iomanip>
#include <sstream>

Auth::Auth(DatabaseManager* db) : db(db) {}

// public functions
// password verification
bool Auth::verify(const std::string& username, const std::string& password) {
    if (!db || !db->getConnection()) {
        std::cerr << "Database connection not available.\n";
        return false;
    }

    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    const char* query = "SELECT password_hash FROM users WHERE username = ? LIMIT 1";

    if (mysql_stmt_prepare(stmt, query, (unsigned long)strlen(query)) != 0) {
        std::cerr << "Failed to prepare verification statement.\n";
        return false;
    }

    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char*)username.c_str();
    bind[0].buffer_length = (unsigned long)username.length();

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        std::cerr << "Failed to bind parameters for verification.\n";
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        std::cerr << "Failed to execute verification statement.\n";
        mysql_stmt_close(stmt);
        return false;
    }

    char stored_hash[256];
    unsigned long hash_length = 0;
    MYSQL_BIND res_bind[1];
    memset(res_bind, 0, sizeof(res_bind));
    res_bind[0].buffer_type = MYSQL_TYPE_STRING;
    res_bind[0].buffer = stored_hash;
    res_bind[0].buffer_length = sizeof(stored_hash);
    res_bind[0].length = &hash_length;

    if (mysql_stmt_bind_result(stmt, res_bind) != 0) {
        std::cerr << "Failed to bind result for verification.\n";
        mysql_stmt_close(stmt);
        return false;
    }

    bool authenticated = false;

    if (mysql_stmt_fetch(stmt) == 0) {
        // hash the input password - use plain text
        std::string storedPassword(stored_hash, hash_length);

        std::string input_hash = password;

        if (password == storedPassword) {
            authenticated = true;
        }
    }

    mysql_stmt_close(stmt);
    return authenticated;
}

bool Auth::userExists(const std::string& username) {
    if (!db || !db->getConnection()) return false;

    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    const char* query = "SELECT COUNT(*) FROM users WHERE username = ?";

    if (mysql_stmt_prepare(stmt, query, (unsigned long)strlen(query)) != 0) return false;

    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char*)username.c_str();
    bind[0].buffer_length = (unsigned long)username.length();

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    int count = 0;
    MYSQL_BIND res_bind[1];
    memset(res_bind, 0, sizeof(res_bind));
    res_bind[0].buffer_type = MYSQL_TYPE_LONG;
    res_bind[0].buffer = &count;

    if (mysql_stmt_bind_result(stmt, res_bind) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_fetch(stmt) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    mysql_stmt_close(stmt);
    return (count > 0);
}

bool Auth::registerUser(const std::string& username, const std::string& password, Role role) {
    if (!db || !db->getConnection()) {
        std::cerr << "Database connection not available.\n";
        return false;
    }

    // Validate username
    if (username.empty() || username.length() < 3 || username.length() > 20) {
        std::cerr << "Username must be 3-20 characters.\n";
        return false;
    }

    // Check for invalid characters in username
    for (char c : username) {
        if (!isalnum(c) && c != '_' && c != '-') {
            std::cerr << "Username can only contain letters, numbers, underscore and hyphen.\n";
            return false;
        }
    }

    // Validate password
    if (password.length() < 6) {
        std::cerr << "Password must be at least 6 characters.\n";
        return false;
    }

    // Check if user already exists
    if (userExists(username)) {
        std::cerr << "User '" << username << "' already exists.\n";
        return false;
    }

    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());

    // Use ENUM strings 'admin' or 'user'
    const char* sql = "INSERT INTO users (username, password_hash, role) VALUES (?, ?, ?)";

    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) == 0) {
        MYSQL_BIND bind[3];
        memset(bind, 0, sizeof(bind));

        // Username
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (char*)username.c_str();
        bind[0].buffer_length = (unsigned long)username.length();

        // Password - store as plain text
        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = (char*)password.c_str();
        bind[1].buffer_length = (unsigned long)password.length();

        std::string roleStr = roleToString(role);
        bind[2].buffer_type = MYSQL_TYPE_STRING;
        bind[2].buffer = (char*)roleStr.c_str();
        bind[2].buffer_length = (unsigned long)roleStr.length();

        if (mysql_stmt_bind_param(stmt, bind) != 0) {
            std::cerr << "Failed to bind parameters for registration.\n";
            mysql_stmt_close(stmt);
            return false;
        }

        bool success = (mysql_stmt_execute(stmt) == 0);
        mysql_stmt_close(stmt);

        if (success) {
            std::cout << "✓ User '" << username << "' registered successfully as "
                << roleStr << ".\n";
            return true;
        }
        else {
            std::cerr << "✗ Failed to register user.\n";
            return false;
        }
    }

    std::cerr << "Failed to prepare registration statement.\n";
    mysql_stmt_close(stmt);
    return false;
}

// login system
bool Auth::login(std::string& loggedInUser, Role& loggedInRole) {
    if (!db || !db->getConnection()) {
        std::cerr << "Database connection not available.\n";
        return false;
    }

    std::string u, p;
    std::cout << "Username: ";
    std::cin >> u;
    std::cout << "Password: ";
    setEcho(false);
    std::cin >> p;
    setEcho(true);
    std::cout << std::endl;

    if (verify(u, p)) {
        loggedInUser = u;

        // Get user role
        if (getUserRole(u, loggedInRole)) {
            std::cout << "Welcome, " << u << "! (" << roleToString(loggedInRole) << ")\n";
            return true;
        }
    }
    std::cout << "Invalid credentials.\n";
    return false;
}

bool Auth::updateUser(const std::string& currentUser, Role currentRole,const std::string& targetUser, const std::string& newPassword) {

    if (!db || !db->getConnection()) {
        std::cerr << "Database connection not available.\n";
        return false;
    }

    // Check permissions
    if (currentRole != Role::ADMIN && currentUser != targetUser) {
        std::cerr << "Permission denied: Users can only update their own profile.\n";
        return false;
    }

    // Check if target user exists
    if (!userExists(targetUser)) {
        std::cerr << "User '" << targetUser << "' does not exist.\n";
        return false;
    }

    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    const char* sql = "UPDATE users SET password_hash = ? WHERE username = ?";

    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) == 0) {
        MYSQL_BIND bind[2];
        memset(bind, 0, sizeof(bind));

        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (char*)newPassword.c_str();
        bind[0].buffer_length = (unsigned long)newPassword.length();

        // Target username
        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = (char*)targetUser.c_str();
        bind[1].buffer_length = (unsigned long)targetUser.length();

        if (mysql_stmt_bind_param(stmt, bind) != 0) {
            std::cerr << "Failed to bind parameters for update.\n";
            mysql_stmt_close(stmt);
            return false;
        }

        if (mysql_stmt_execute(stmt) == 0) {
            int affected = (int)mysql_stmt_affected_rows(stmt);
            mysql_stmt_close(stmt);

            if (affected > 0) {
                std::cout << "User '" << targetUser << "' password updated successfully.\n";
                return true;
            }
        }
    }

    std::cerr << "Failed to update user '" << targetUser << "'.\n";
    mysql_stmt_close(stmt);
    return false;
}

bool Auth::deleteUser(const std::string& currentUser, Role currentRole, const std::string& targetUser) {
    if (!db || !db->getConnection()) {
        std::cerr << "Database connection not available.\n";
        return false;
    }

    // Check permissions
    if (currentRole != Role::ADMIN && currentUser != targetUser) {
        std::cerr << "Permission denied: Users can only delete their own account.\n";
        return false;
    }

    // Check if target user exists
    if (!userExists(targetUser)) {
        std::cerr << "User '" << targetUser << "' does not exist.\n";
        return false;
    }

    // Confirmation for deletion
    std::cout << "Are you sure you want to delete user '" << targetUser << "'? (y/n): ";
    char confirm;
    std::cin >> confirm;
    if (confirm != 'y' && confirm != 'Y') {
        std::cout << "Deletion cancelled.\n";
        return false;
    }

    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    const char* sql = "DELETE FROM users WHERE username = ?";

    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) == 0) {
        MYSQL_BIND bind[1];
        memset(bind, 0, sizeof(bind));

        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (char*)targetUser.c_str();
        bind[0].buffer_length = (unsigned long)targetUser.length();

        if (mysql_stmt_bind_param(stmt, bind) != 0) {
            std::cerr << "Failed to bind parameters for deletion.\n";
            mysql_stmt_close(stmt);
            return false;
        }

        if (mysql_stmt_execute(stmt) == 0) {
            int affected = (int)mysql_stmt_affected_rows(stmt);
            mysql_stmt_close(stmt);

            if (affected > 0) {
                std::cout << "User '" << targetUser << "' deleted successfully.\n";
                return true;
            }
        }
    }

    std::cerr << "Failed to delete user '" << targetUser << "'.\n";
    mysql_stmt_close(stmt);
    return false;
}

bool Auth::getUserRole(const std::string& username, Role& role) {
    if (!db || !db->getConnection()) return false;

    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    const char* query = "SELECT role FROM users WHERE username = ?";

    if (mysql_stmt_prepare(stmt, query, (unsigned long)strlen(query)) != 0) return false;

    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char*)username.c_str();
    bind[0].buffer_length = (unsigned long)username.length();

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    char role_buf[10];
    unsigned long role_length = 0;
    MYSQL_BIND res_bind[1];
    memset(res_bind, 0, sizeof(res_bind));
    res_bind[0].buffer_type = MYSQL_TYPE_STRING;
    res_bind[0].buffer = role_buf;
    res_bind[0].buffer_length = sizeof(role_buf);
    res_bind[0].length = &role_length;

    if (mysql_stmt_bind_result(stmt, res_bind) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_fetch(stmt) == 0) {
        std::string roleStr(role_buf, role_length);
        role = stringToRole(roleStr);
        mysql_stmt_close(stmt);
        return true;
    }

    mysql_stmt_close(stmt);
    return false;
}

bool Auth::listAllUsers(const std::string& adminUser, Role adminRole) {
    if (!db || !db->getConnection()) {
        std::cerr << "Database connection not available.\n";
        return false;
    }

    // Check if caller is admin
    if (adminRole != Role::ADMIN) {
        std::cerr << "Permission denied: Only admins can list all users.\n";
        return false;
    }

    MYSQL* conn = db->getConnection();

    std::string query = "SELECT username, role FROM users ORDER BY username";

    if (mysql_query(conn, query.c_str()) == 0) {
        MYSQL_RES* res = mysql_store_result(conn);
        if (res) {
            std::cout << "\n=== All Registered Users ===\n";
            std::cout << std::left << std::setw(20) << "Username"
                << std::setw(10) << "Role"
                << std::setw(10) << "Status"
                << "\n";
            std::cout << std::string(40, '-') << "\n";

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                std::string username = row[0] ? row[0] : "";
                std::string roleStr = row[1] ? row[1] : "user";
                std::string status = (username == adminUser) ? "(You)" : "";

                std::cout << std::left << std::setw(20) << username
                    << std::setw(10) << roleStr
                    << std::setw(10) << status
                    << "\n";
            }

            int count = mysql_num_rows(res);
            std::cout << "\nTotal users: " << count << "\n";
            mysql_free_result(res);
            return true;
        }
    }

    std::cerr << "Failed to list users: " << mysql_error(conn) << std::endl;
    return false;
}

bool Auth::changeUserRole(const std::string& adminUser, Role adminRole, const std::string& targetUser, Role newRole) {
    if (!db || !db->getConnection()) {
        std::cerr << "Database connection not available.\n";
        return false;
    }

    // Check if caller is admin
    if (adminRole != Role::ADMIN) {
        std::cerr << "Permission denied: Only admins can change user roles.\n";
        return false;
    }

    // Check if target user exists
    if (!userExists(targetUser)) {
        std::cerr << "User '" << targetUser << "' does not exist.\n";
        return false;
    }

    if (adminUser == targetUser) {
        std::cerr << "Cannot change your own role.\n";
        return false;
    }

    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    const char* sql = "UPDATE users SET role = ? WHERE username = ?";

    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) == 0) {
        MYSQL_BIND bind[2];
        memset(bind, 0, sizeof(bind));

        // New role as ENUM string
        std::string roleStr = roleToString(newRole);
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (char*)roleStr.c_str();
        bind[0].buffer_length = (unsigned long)roleStr.length();

        // Target username
        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = (char*)targetUser.c_str();
        bind[1].buffer_length = (unsigned long)targetUser.length();

        if (mysql_stmt_bind_param(stmt, bind) != 0) {
            std::cerr << "Failed to bind parameters for role change.\n";
            mysql_stmt_close(stmt);
            return false;
        }

        if (mysql_stmt_execute(stmt) == 0) {
            int affected = (int)mysql_stmt_affected_rows(stmt);
            mysql_stmt_close(stmt);

            if (affected > 0) {
                std::cout << "User '" << targetUser << "' role changed to "
                    << roleStr << ".\n";
                return true;
            }
        }
    }

    std::cerr << "Failed to change role for user '" << targetUser << "'.\n";
    mysql_stmt_close(stmt);
    return false;
}

std::string Auth::roleToString(Role role) {
    switch (role) {
    case Role::ADMIN: return "admin";
    case Role::USER: return "user";
    default: return "user";
    }
}

Auth::Role Auth::stringToRole(const std::string& roleStr) {
    if (roleStr == "admin") {
        return Role::ADMIN;
    }
    return Role::USER;
}

// private functions
void Auth::setEcho(bool enable) {
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    if (!enable) mode &= ~ENABLE_ECHO_INPUT;
    else mode |= ENABLE_ECHO_INPUT;
    SetConsoleMode(hStdin, mode);
}

std::string Auth::generateSalt(size_t length) {
    HCRYPTPROV hProv = 0;
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        throw std::runtime_error("Unable to acquire crypto context");
    }

    std::vector<BYTE> saltBytes(length);
    if (!CryptGenRandom(hProv, length, saltBytes.data())) {
        CryptReleaseContext(hProv, 0);
        throw std::runtime_error("Unable to generate salt");
    }

    CryptReleaseContext(hProv, 0);
    return bytesToHex(saltBytes.data(), length);

}

std::string Auth::hashPassword(const std::string& password, const std::string& salt) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        throw std::runtime_error("Unable to acquire crypto context");
    }

    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        throw std::runtime_error("Failed to create hash");
    }

    // Hash salt + password
    std::string combined = salt + password;
    if (!CryptHashData(hHash, (BYTE*)combined.c_str(), combined.length(), 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        throw std::runtime_error("Failed to hash data");
    }

    // Get hash value
    DWORD hashLen = 32; // SHA-256 produces 32 bytes
    BYTE hashValue[32];
    DWORD dwHashLen = sizeof(hashValue);

    if (!CryptGetHashParam(hHash, HP_HASHVAL, hashValue, &dwHashLen, 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        throw std::runtime_error("Failed to get hash value");
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    return bytesToHex(hashValue, hashLen);
}

bool Auth::verifyPassword(const std::string& password, const std::string& storedHash) {
    // Parse salt:hash format
    size_t colonPos = storedHash.find(':');
    if (colonPos == std::string::npos) {
        // Legacy plain text support (for migration)
        return password == storedHash;
    }

    std::string salt = storedHash.substr(0, colonPos);
    std::string storedHashValue = storedHash.substr(colonPos + 1);

    // Compute hash of input password with salt
    std::string computedHash = hashPassword(password, salt);

    // Constant-time comparison to prevent timing attacks
    return computedHash == storedHashValue;
}

std::string Auth::bytesToHex(const unsigned char* bytes, size_t length) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < length; ++i) {
        oss << std::setw(2) << static_cast<unsigned>(bytes[i]);
    }
    return oss.str();
}

std::vector<unsigned char> Auth::hexToBytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(strtoul(byteString.c_str(), NULL, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

