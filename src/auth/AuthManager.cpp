#include "AuthManager.h"
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#ifdef USE_AUTH
#include <jwt/jwt.hpp>
#endif
#include <iostream>
#include <sstream>
#include <iomanip>
#include <regex>
#include <chrono>

AuthManager::AuthManager(std::shared_ptr<DatabaseManager> db) 
    : database(db)
    , maxFailedAttempts(5)
    , lockoutDurationMinutes(30)
    , sessionTimeoutMinutes(1440) // 24 hours
    , refreshTokenTimeoutDays(30)
    , requireEmailVerification(true)
    , requireTwoFactor(false)
    , minPasswordLength(8)
    , requireUppercase(true)
    , requireLowercase(true)
    , requireNumbers(true)
    , requireSpecialChars(true)
{
    // Initialize JWT settings
    jwtSecret = "your-super-secret-jwt-key-change-this-in-production";
    jwtIssuer = "SlotMachine-Auth";
    
    // Load common passwords list (simplified)
    commonPasswords = {
        "password", "123456", "password123", "admin", "qwerty",
        "letmein", "welcome", "monkey", "dragon", "master"
    };
}

AuthManager::~AuthManager() {
    Shutdown();
}

bool AuthManager::Initialize() {
    std::cout << "Initializing Authentication Manager..." << std::endl;
    
    if (!database || !database->IsConnected()) {
        std::cerr << "Database not available for authentication" << std::endl;
        return false;
    }
    
    std::cout << "Authentication Manager initialized successfully" << std::endl;
    return true;
}

void AuthManager::Shutdown() {
    std::cout << "Shutting down Authentication Manager..." << std::endl;
    
    // Clear sensitive data
    loginAttempts.clear();
    registrationAttempts.clear();
    commonPasswords.clear();
    jwtSecret.clear();
}

AuthResult AuthManager::Login(const LoginRequest& request, AuthToken& token) {
    // Rate limiting check
    if (IsRateLimited(request.ipAddress, "login")) {
        LogSecurityEvent(0, "rate_limit_exceeded", "Login rate limit exceeded", request.ipAddress, request.userAgent);
        return AuthResult::RATE_LIMITED;
    }
    
    // Get user by username
    User user = database->GetUserByUsername(request.username);
    if (user.id == 0) {
        UpdateRateLimit(request.ipAddress, "login");
        return AuthResult::USER_NOT_FOUND;
    }
    
    // Check if user is locked
    if (IsUserLocked(user.id)) {
        LogSecurityEvent(user.id, "login_attempt_locked", "Login attempt on locked account", request.ipAddress, request.userAgent);
        return AuthResult::USER_LOCKED;
    }
    
    // Check if user is active
    if (!user.isActive) {
        return AuthResult::USER_INACTIVE;
    }
    
    // Verify password
    if (!VerifyPassword(request.password, user.passwordHash, user.salt)) {
        database->IncrementFailedLogins(user.id);
        
        // Lock user if too many failed attempts
        if (user.failedLoginAttempts + 1 >= maxFailedAttempts) {
            auto lockUntil = std::chrono::system_clock::now() + std::chrono::minutes(lockoutDurationMinutes);
            auto lockTime = std::chrono::system_clock::to_time_t(lockUntil);
            
            std::stringstream ss;
            ss << std::put_time(std::gmtime(&lockTime), "%Y-%m-%d %H:%M:%S");
            database->LockUser(user.id, ss.str());
            
            LogSecurityEvent(user.id, "account_locked", "Account locked due to failed login attempts", request.ipAddress, request.userAgent);
        }
        
        UpdateRateLimit(request.ipAddress, "login");
        return AuthResult::INVALID_CREDENTIALS;
    }
    
    // Check email verification
    if (requireEmailVerification && !user.isVerified) {
        return AuthResult::EMAIL_NOT_VERIFIED;
    }
    
    // Check two-factor authentication
    if (user.isTwoFactorEnabled) {
        if (request.twoFactorCode.empty()) {
            return AuthResult::TWO_FACTOR_REQUIRED;
        }
        
        if (!ValidateTwoFactorCode(user.twoFactorSecret, request.twoFactorCode)) {
            LogSecurityEvent(user.id, "invalid_2fa", "Invalid two-factor authentication code", request.ipAddress, request.userAgent);
            return AuthResult::INVALID_TWO_FACTOR;
        }
    }
    
    // Reset failed login attempts
    database->ResetFailedLogins(user.id);
    
    // Create session
    std::string sessionToken = database->CreateUserSession(user.id, request.ipAddress, request.userAgent);
    if (sessionToken.empty()) {
        return AuthResult::DATABASE_ERROR;
    }
    
    // Generate JWT token
    std::string jwtToken = GenerateJWT(user.id, user.username, UserRole::PLAYER);
    
    // Prepare auth token response
    token.accessToken = jwtToken;
    token.refreshToken = sessionToken; // Using session token as refresh token
    token.tokenType = "Bearer";
    token.expiresIn = sessionTimeoutMinutes * 60;
    token.scope = "user";
    token.userId = std::to_string(user.id);
    
    // Log successful login
    LogSecurityEvent(user.id, "login_success", "User logged in successfully", request.ipAddress, request.userAgent);
    
    if (onLogin) {
        onLogin(user.id, request.ipAddress);
    }
    
    return AuthResult::SUCCESS;
}

AuthResult AuthManager::Register(const RegisterRequest& request, std::string& userId) {
    // Rate limiting check
    if (IsRateLimited(request.ipAddress, "register")) {
        return AuthResult::RATE_LIMITED;
    }
    
    // Validate input
    if (request.username.empty() || request.email.empty() || request.password.empty()) {
        return AuthResult::INVALID_CREDENTIALS;
    }
    
    if (request.password != request.confirmPassword) {
        return AuthResult::INVALID_CREDENTIALS;
    }
    
    // Validate password policy
    if (!ValidatePasswordPolicy(request.password)) {
        return AuthResult::INVALID_CREDENTIALS;
    }
    
    // Check if username already exists
    User existingUser = database->GetUserByUsername(request.username);
    if (existingUser.id != 0) {
        UpdateRateLimit(request.ipAddress, "register");
        return AuthResult::INVALID_CREDENTIALS;
    }
    
    // Check if email already exists
    existingUser = database->GetUserByEmail(request.email);
    if (existingUser.id != 0) {
        UpdateRateLimit(request.ipAddress, "register");
        return AuthResult::INVALID_CREDENTIALS;
    }
    
    // Create new user
    User newUser = {};
    newUser.username = request.username;
    newUser.email = request.email;
    newUser.salt = GenerateSalt();
    newUser.passwordHash = HashPassword(request.password, newUser.salt);
    newUser.balance = 0.0;
    newUser.dailyLimit = 1000.0;
    newUser.isActive = true;
    newUser.isVerified = !requireEmailVerification;
    newUser.deviceFingerprint = request.deviceFingerprint;
    newUser.failedLoginAttempts = 0;
    newUser.isTwoFactorEnabled = false;
    
    if (!database->CreateUser(newUser)) {
        return AuthResult::DATABASE_ERROR;
    }
    
    // Get the created user to get the ID
    User createdUser = database->GetUserByUsername(request.username);
    if (createdUser.id == 0) {
        return AuthResult::DATABASE_ERROR;
    }
    
    userId = std::to_string(createdUser.id);
    
    // Send verification email if required
    if (requireEmailVerification) {
        SendVerificationEmail(createdUser.id);
    }
    
    // Log registration
    LogSecurityEvent(createdUser.id, "user_registered", "New user registered", request.ipAddress, request.userAgent);
    
    UpdateRateLimit(request.ipAddress, "register");
    return AuthResult::SUCCESS;
}

std::string AuthManager::HashPassword(const std::string& password, const std::string& salt) {
    std::string saltedPassword = password + salt;
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, saltedPassword.c_str(), saltedPassword.length());
    SHA256_Final(hash, &sha256);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

std::string AuthManager::GenerateSalt() {
    const int saltLength = 32;
    unsigned char salt[saltLength];
    
    if (RAND_bytes(salt, saltLength) != 1) {
        // Fallback to less secure method
        srand(time(nullptr));
        for (int i = 0; i < saltLength; i++) {
            salt[i] = rand() % 256;
        }
    }
    
    std::stringstream ss;
    for (int i = 0; i < saltLength; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)salt[i];
    }
    
    return ss.str();
}

bool AuthManager::VerifyPassword(const std::string& password, const std::string& hash, const std::string& salt) {
    std::string computedHash = HashPassword(password, salt);
    return computedHash == hash;
}

bool AuthManager::ValidatePasswordPolicy(const std::string& password) {
    if (password.length() < minPasswordLength) {
        return false;
    }
    
    // Check for common passwords
    for (const auto& common : commonPasswords) {
        if (password == common) {
            return false;
        }
    }
    
    bool hasUpper = false, hasLower = false, hasNumber = false, hasSpecial = false;
    
    for (char c : password) {
        if (std::isupper(c)) hasUpper = true;
        else if (std::islower(c)) hasLower = true;
        else if (std::isdigit(c)) hasNumber = true;
        else if (std::ispunct(c)) hasSpecial = true;
    }
    
    if (requireUppercase && !hasUpper) return false;
    if (requireLowercase && !hasLower) return false;
    if (requireNumbers && !hasNumber) return false;
    if (requireSpecialChars && !hasSpecial) return false;
    
    return true;
}

bool AuthManager::IsRateLimited(const std::string& identifier, const std::string& action) {
    auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    
    std::string key = identifier + ":" + action;
    auto& attempts = (action == "login") ? loginAttempts[key] : registrationAttempts[key];
    
    // Remove old attempts (older than 1 hour)
    attempts.erase(std::remove_if(attempts.begin(), attempts.end(),
        [currentTime](uint64_t timestamp) {
            return currentTime - timestamp > 3600000; // 1 hour
        }), attempts.end());
    
    // Check rate limits
    if (action == "login") {
        return attempts.size() >= 10; // Max 10 login attempts per hour
    } else if (action == "register") {
        return attempts.size() >= 3; // Max 3 registration attempts per hour
    }
    
    return false;
}

void AuthManager::UpdateRateLimit(const std::string& identifier, const std::string& action) {
    auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    
    std::string key = identifier + ":" + action;
    if (action == "login") {
        loginAttempts[key].push_back(currentTime);
    } else if (action == "register") {
        registrationAttempts[key].push_back(currentTime);
    }
}

std::string AuthManager::GenerateJWT(int userId, const std::string& username, UserRole role) {
#ifdef USE_AUTH
    // Simplified JWT implementation - in production use proper JWT library
    std::stringstream ss;
    ss << "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9."; // Header
    
    // Payload (base64 encoded)
    std::string payload = "{\"user_id\":\"" + std::to_string(userId) + 
                         "\",\"username\":\"" + username + 
                         "\",\"role\":\"" + std::to_string(static_cast<int>(role)) + 
                         "\",\"exp\":" + std::to_string(time(nullptr) + sessionTimeoutMinutes * 60) + "}";
    
    // Simple base64 encoding (simplified for demo)
    ss << payload << ".signature";
    
    return ss.str();
#else
    return "mock_jwt_token_" + std::to_string(userId);
#endif
}

bool AuthManager::ValidateSession(const std::string& sessionToken) {
    return database->ValidateSession(sessionToken);
}

bool AuthManager::IsUserLocked(int userId) {
    User user = database->GetUser(userId);
    if (user.id == 0) return false;
    
    if (user.lockedUntil.empty()) return false;
    
    // Parse lock time and check if still locked
    // This is simplified - in production, you'd use proper date parsing
    auto now = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);
    
    // For simplicity, assuming lock is expired if more than lockout duration has passed
    return user.failedLoginAttempts >= maxFailedAttempts;
}

void AuthManager::LogSecurityEvent(int userId, const std::string& eventType, const std::string& description, 
                                  const std::string& ipAddress, const std::string& userAgent) {
    SecurityEvent event = {};
    event.userId = userId;
    event.eventType = eventType;
    event.severity = "medium";
    event.description = description;
    event.ipAddress = ipAddress;
    event.userAgent = userAgent;
    
    database->LogSecurityEvent(event);
}

AuthResult AuthManager::SendVerificationEmail(int userId) {
    // This would integrate with an email service
    // For now, just log the action
    LogSecurityEvent(userId, "verification_email_sent", "Email verification sent");
    return AuthResult::SUCCESS;
}

User AuthManager::GetUserInfo(int userId) {
    return database->GetUser(userId);
}

bool AuthManager::ValidateSession(const std::string& sessionToken) {
    return database->ValidateSession(sessionToken);
}

AuthResult AuthManager::Logout(const std::string& sessionToken) {
    if (!database->InvalidateSession(sessionToken)) {
        return AuthResult::INVALID_SESSION;
    }
    
    if (onLogout) {
        // Get user info from session before invalidating
        UserSession session = database->GetSession(sessionToken);
        onLogout(session.userId, session.ipAddress);
    }
    
    return AuthResult::SUCCESS;
}