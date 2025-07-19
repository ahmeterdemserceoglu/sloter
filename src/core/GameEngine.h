#pragma once
#include <vector>
#include <random>
#include <chrono>
#include <memory>

enum class SlotSymbol {
    CHERRY = 0,
    LEMON,
    ORANGE,
    PLUM,
    BELL,
    BAR,
    SEVEN,
    DIAMOND,
    WILD,
    SCATTER
};

struct SpinResult {
    std::vector<std::vector<SlotSymbol>> reels;
    double winAmount;
    bool isWin;
    std::vector<int> winLines;
    bool isBonusTriggered;
    bool isJackpot;
};

struct GameStats {
    double totalBet;
    double totalWin;
    int totalSpins;
    int winningSpins;
    double rtp; // Return to Player percentage
};

class GameEngine {
private:
    // Game configuration
    static const int REEL_COUNT = 5;
    static const int SYMBOL_COUNT = 3;
    static const int PAYLINES = 25;
    
    // RNG system
    std::mt19937_64 rng;
    std::uniform_int_distribution<int> symbolDist;
    
    // Game state
    double currentBalance;
    double currentBet;
    GameStats stats;
    bool isSpinning;
    
    // Paytable
    std::vector<std::vector<double>> paytable;
    
    // Security
    uint64_t lastSpinTime;
    std::vector<uint64_t> spinHistory;
    
    // Anti-cheat measures
    bool ValidateSpinTiming();
    bool DetectPatternAbuse();
    void UpdateSpinHistory();
    
    // Game logic
    std::vector<SlotSymbol> GenerateReel();
    SpinResult CalculateWin(const std::vector<std::vector<SlotSymbol>>& reels);
    double CalculateLineWin(const std::vector<SlotSymbol>& line);
    bool CheckWinLine(const std::vector<std::vector<SlotSymbol>>& reels, int lineIndex);
    
public:
    GameEngine();
    ~GameEngine();
    
    bool Initialize();
    void Update();
    void Shutdown();
    
    // Game actions
    SpinResult Spin(double betAmount);
    bool CanSpin() const;
    
    // Balance management
    void SetBalance(double balance) { currentBalance = balance; }
    double GetBalance() const { return currentBalance; }
    void SetBet(double bet) { currentBet = bet; }
    double GetBet() const { return currentBet; }
    
    // Statistics
    const GameStats& GetStats() const { return stats; }
    void ResetStats();
    
    // Security
    bool IsSecure() const;
    void ReportSuspiciousActivity();
};