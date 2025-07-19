#include "SecurityManager.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

#ifdef ANDROID
#include <android/log.h>
#include <sys/system_properties.h>
#include <unistd.h>
#include <sys/ptrace.h>
#define LOG_TAG "SecurityManager"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...) printf(__VA_ARGS__)
#define LOGW(...) printf(__VA_ARGS__)
#define LOGE(...) printf(__VA_ARGS__)
#endif

SecurityManager::SecurityManager() 
    : lastSecurityCheck(0)
    , isSecurityEnabled(true)
    , memoryChecksum(0)
    , isRooted(false)
    , isEmulator(false)
    , isDebuggerAttached(false)
{
    securityLog.reserve(1000);
}

SecurityManager::~SecurityManager() {
    Shutdown();
}

bool SecurityManager::Initialize() {
    LOGI("Initializing Security Manager...\n");
    
    // Generate device fingerprint
    deviceFingerprint = GenerateDeviceFingerprint();
    
    // Perform initial security checks
    isRooted = DetectRootAccess();
    isEmulator = DetectEmulator();
    isDebuggerAttached = DetectDebugger();
    
    if (isRooted) {
        LogSecurityEvent(SecurityThreat::ROOT_DETECTED, "Root access detected on device");
        LOGW("WARNING: Root access detected!\n");
    }
    
    if (isEmulator) {
        LogSecurityEvent(SecurityThreat::EMULATOR_DETECTED, "Running on emulator");
        LOGW("WARNING: Emulator detected!\n");
    }
    
    if (isDebuggerAttached) {
        LogSecurityEvent(SecurityThreat::DEBUGGER_DETECTED, "Debugger attached to process");
        LOGW("WARNING: Debugger detected!\n");
    }
    
    // Initialize memory protection
    memoryChecksum = CalculateMemoryChecksum();
    
    LOGI("Security Manager initialized successfully\n");
    return true;
}

void SecurityManager::Shutdown() {
    LOGI("Shutting down Security Manager...\n");
    
    // Clear sensitive data
    securityLog.clear();
    checksumCache.clear();
    actionHistory.clear();
    protectedMemory.clear();
    
    deviceFingerprint.clear();
}

bool SecurityManager::PerformSecurityCheck() {
    if (!isSecurityEnabled) return true;
    
    auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    
    // Perform security checks every 5 seconds
    if (currentTime - lastSecurityCheck < 5000) {
        return true;
    }
    
    lastSecurityCheck = currentTime;
    
    // Memory integrity check
    if (!CheckMemoryIntegrity()) {
        LogSecurityEvent(SecurityThreat::MEMORY_TAMPERING, "Memory integrity violation detected");
        LOGE("SECURITY BREACH: Memory tampering detected!\n");
        return false;
    }
    
    // Debugger detection
    if (DetectDebugger() && !isDebuggerAttached) {
        isDebuggerAttached = true;
        LogSecurityEvent(SecurityThreat::DEBUGGER_DETECTED, "Debugger attached during runtime");
        LOGE("SECURITY BREACH: Debugger attached!\n");
        return false;
    }
    
    // Code integrity check
    if (!VerifyCodeIntegrity()) {
        LogSecurityEvent(SecurityThreat::INJECTION_ATTEMPT, "Code integrity violation");
        LOGE("SECURITY BREACH: Code tampering detected!\n");
        return false;
    }
    
    return true;
}

bool SecurityManager::ValidateGameAction(const std::string& action) {
    // Rate limiting check
    if (IsRateLimited(action)) {
        LogSecurityEvent(SecurityThreat::SPEED_HACK, "Rate limit exceeded for action: " + action);
        return false;
    }
    
    UpdateActionHistory(action);
    return true;
}

bool SecurityManager::ValidatePaymentRequest(double amount) {
    // Validate payment amount
    if (amount < 0 || amount > 10000) { // Max $10,000 per transaction
        LogSecurityEvent(SecurityThreat::INJECTION_ATTEMPT, "Invalid payment amount: " + std::to_string(amount));
        return false;
    }
    
    // Check for payment rate limiting
    if (IsRateLimited("payment")) {
        LogSecurityEvent(SecurityThreat::SPEED_HACK, "Payment rate limit exceeded");
        return false;
    }
    
    UpdateActionHistory("payment");
    return true;
}

bool SecurityManager::ValidateUserInput(const std::string& input) {
    // Check for injection attempts
    std::vector<std::string> dangerousPatterns = {
        "SELECT", "INSERT", "UPDATE", "DELETE", "DROP",
        "<script", "javascript:", "eval(", "exec(",
        "../", "..\\", "cmd.exe", "/bin/sh"
    };
    
    std::string upperInput = input;
    std::transform(upperInput.begin(), upperInput.end(), upperInput.begin(), ::toupper);
    
    for (const auto& pattern : dangerousPatterns) {
        if (upperInput.find(pattern) != std::string::npos) {
            LogSecurityEvent(SecurityThreat::INJECTION_ATTEMPT, "Dangerous pattern detected: " + pattern);
            return false;
        }
    }
    
    return true;
}

bool SecurityManager::CheckMemoryIntegrity() {
    uint32_t currentChecksum = CalculateMemoryChecksum();
    
    if (memoryChecksum != 0 && currentChecksum != memoryChecksum) {
        return false;
    }
    
    memoryChecksum = currentChecksum;
    return true;
}

bool SecurityManager::DetectDebugger() {
#ifdef ANDROID
    // Check if ptrace is being used
    if (ptrace(PTRACE_TRACEME, 0, 1, 0) == -1) {
        return true; // Debugger detected
    }
    
    // Check for common debugger processes
    FILE* fp = fopen("/proc/self/status", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "TracerPid:")) {
                int tracerPid = 0;
                sscanf(line, "TracerPid: %d", &tracerPid);
                fclose(fp);
                return tracerPid != 0;
            }
        }
        fclose(fp);
    }
#endif
    
    return false;
}

bool SecurityManager::DetectRootAccess() {
#ifdef ANDROID
    // Check for common root files
    std::vector<std::string> rootFiles = {
        "/system/app/Superuser.apk",
        "/sbin/su",
        "/system/bin/su",
        "/system/xbin/su",
        "/data/local/xbin/su",
        "/data/local/bin/su",
        "/system/sd/xbin/su",
        "/system/bin/failsafe/su",
        "/data/local/su"
    };
    
    for (const auto& file : rootFiles) {
        if (access(file.c_str(), F_OK) == 0) {
            return true;
        }
    }
    
    // Check for root management apps
    std::vector<std::string> rootApps = {
        "com.noshufou.android.su",
        "com.thirdparty.superuser",
        "eu.chainfire.supersu",
        "com.koushikdutta.superuser"
    };
    
    // This would require additional Android-specific code to check installed packages
#endif
    
    return false;
}

bool SecurityManager::DetectEmulator() {
#ifdef ANDROID
    char prop_value[PROP_VALUE_MAX];
    
    // Check build properties that indicate emulator
    if (__system_property_get("ro.kernel.qemu", prop_value) > 0) {
        return true;
    }
    
    if (__system_property_get("ro.product.model", prop_value) > 0) {
        std::string model(prop_value);
        if (model.find("sdk") != std::string::npos || 
            model.find("Emulator") != std::string::npos ||
            model.find("Android SDK") != std::string::npos) {
            return true;
        }
    }
    
    if (__system_property_get("ro.product.manufacturer", prop_value) > 0) {
        std::string manufacturer(prop_value);
        if (manufacturer == "Genymotion" || manufacturer == "unknown") {
            return true;
        }
    }
#endif
    
    return false;
}

std::string SecurityManager::GenerateDeviceFingerprint() {
    std::stringstream ss;
    
#ifdef ANDROID
    char prop_value[PROP_VALUE_MAX];
    
    // Collect device properties
    if (__system_property_get("ro.product.model", prop_value) > 0) {
        ss << prop_value << "|";
    }
    
    if (__system_property_get("ro.product.manufacturer", prop_value) > 0) {
        ss << prop_value << "|";
    }
    
    if (__system_property_get("ro.build.fingerprint", prop_value) > 0) {
        ss << prop_value << "|";
    }
    
    if (__system_property_get("ro.serialno", prop_value) > 0) {
        ss << prop_value << "|";
    }
#else
    // Desktop fingerprinting would use different methods
    ss << "desktop|unknown|";
#endif
    
    // Add timestamp for uniqueness
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    ss << time_t;
    
    return ss.str();
}

uint32_t SecurityManager::CalculateMemoryChecksum() {
    // Simple checksum calculation for critical memory regions
    // In a real implementation, this would be more sophisticated
    uint32_t checksum = 0;
    
    // This is a simplified example - real implementation would
    // checksum critical game data structures
    for (size_t i = 0; i < sizeof(SecurityManager); i++) {
        checksum += reinterpret_cast<const uint8_t*>(this)[i];
    }
    
    return checksum;
}

bool SecurityManager::VerifyCodeIntegrity() {
    // Code integrity verification
    // In a real implementation, this would verify executable sections
    return true; // Simplified for this example
}

bool SecurityManager::CheckFileIntegrity() {
    // File integrity checking
    // Would verify game assets haven't been modified
    return true; // Simplified for this example
}

void SecurityManager::LogSecurityEvent(SecurityThreat threat, const std::string& description) {
    SecurityEvent event;
    event.threat = threat;
    event.description = description;
    event.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    event.deviceInfo = deviceFingerprint;
    
    securityLog.push_back(event);
    
    // Keep only last 1000 events
    if (securityLog.size() > 1000) {
        securityLog.erase(securityLog.begin());
    }
    
    LOGW("Security Event: %s\n", description.c_str());
}

bool SecurityManager::IsRateLimited(const std::string& action) {
    auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    
    auto& history = actionHistory[action];
    
    // Remove old entries (older than 1 minute)
    history.erase(std::remove_if(history.begin(), history.end(),
        [currentTime](uint64_t timestamp) {
            return currentTime - timestamp > 60000; // 1 minute
        }), history.end());
    
    // Check rate limits based on action type
    if (action == "spin") {
        return history.size() >= 100; // Max 100 spins per minute
    } else if (action == "payment") {
        return history.size() >= 10; // Max 10 payments per minute
    } else {
        return history.size() >= 50; // Default: 50 actions per minute
    }
}

void SecurityManager::UpdateActionHistory(const std::string& action) {
    auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    
    actionHistory[action].push_back(currentTime);
}

bool SecurityManager::IsDeviceSecure() const {
    return !isRooted && !isEmulator && !isDebuggerAttached;
}

void SecurityManager::ClearSecurityLog() {
    securityLog.clear();
}

bool SecurityManager::HasSecurityViolations() const {
    return !securityLog.empty();
}

void SecurityManager::TriggerSecurityLockdown() {
    LOGE("SECURITY LOCKDOWN TRIGGERED!\n");
    isSecurityEnabled = false;
    
    LogSecurityEvent(SecurityThreat::INJECTION_ATTEMPT, "Security lockdown triggered");
    
    // In a real implementation, this would:
    // - Disable all game functions
    // - Send alert to security service
    // - Log detailed forensic information
}

void SecurityManager::ReportSecurityBreach(const std::string& details) {
    LogSecurityEvent(SecurityThreat::INJECTION_ATTEMPT, "Security breach reported: " + details);
    
    // In a real implementation, this would send data to a security service
    LOGE("Security breach reported: %s\n", details.c_str());
}

void SecurityManager::SetSecurityLevel(int level) {
    switch (level) {
        case 1: // Low
            isSecurityEnabled = true;
            break;
        case 2: // Medium
            isSecurityEnabled = true;
            break;
        case 3: // High
            isSecurityEnabled = true;
            break;
        default:
            isSecurityEnabled = true;
            break;
    }
    
    LOGI("Security level set to: %d\n", level);
}