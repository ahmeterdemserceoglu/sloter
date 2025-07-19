#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <memory>

enum class SecurityThreat {
    NONE = 0,
    MEMORY_TAMPERING,
    SPEED_HACK,
    PATTERN_ABUSE,
    INJECTION_ATTEMPT,
    DEBUGGER_DETECTED,
    ROOT_DETECTED,
    EMULATOR_DETECTED
};

struct SecurityEvent {
    SecurityThreat threat;
    std::string description;
    uint64_t timestamp;
    std::string deviceInfo;
};

class SecurityManager {
private:
    // Anti-cheat measures
    std::vector<SecurityEvent> securityLog;
    std::unordered_map<std::string, uint64_t> checksumCache;
    uint64_t lastSecurityCheck;
    bool isSecurityEnabled;
    
    // Memory protection
    std::vector<void*> protectedMemory;
    uint32_t memoryChecksum;
    
    // Device fingerprinting
    std::string deviceFingerprint;
    bool isRooted;
    bool isEmulator;
    bool isDebuggerAttached;
    
    // Rate limiting
    std::unordered_map<std::string, std::vector<uint64_t>> actionHistory;
    
    // Internal security functions
    bool CheckMemoryIntegrity();
    bool DetectDebugger();
    bool DetectRootAccess();
    bool DetectEmulator();
    bool ValidateDeviceFingerprint();
    uint32_t CalculateMemoryChecksum();
    void LogSecurityEvent(SecurityThreat threat, const std::string& description);
    bool IsRateLimited(const std::string& action);
    void UpdateActionHistory(const std::string& action);
    
    // Anti-tampering
    bool VerifyCodeIntegrity();
    bool CheckFileIntegrity();
    
public:
    SecurityManager();
    ~SecurityManager();
    
    bool Initialize();
    void Shutdown();
    
    // Main security check - called frequently
    bool PerformSecurityCheck();
    
    // Specific security validations
    bool ValidateGameAction(const std::string& action);
    bool ValidatePaymentRequest(double amount);
    bool ValidateUserInput(const std::string& input);
    
    // Device security
    bool IsDeviceSecure() const;
    std::string GetDeviceFingerprint() const { return deviceFingerprint; }
    
    // Security reporting
    std::vector<SecurityEvent> GetSecurityLog() const { return securityLog; }
    void ClearSecurityLog();
    bool HasSecurityViolations() const;
    
    // Emergency functions
    void TriggerSecurityLockdown();
    void ReportSecurityBreach(const std::string& details);
    
    // Configuration
    void SetSecurityLevel(int level); // 1=Low, 2=Medium, 3=High
    bool IsSecurityEnabled() const { return isSecurityEnabled; }
};