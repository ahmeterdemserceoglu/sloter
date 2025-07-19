#include "GameEngine.h"
#include <algorithm>
#include <ctime>

GameEngine::GameEngine() 
    : currentBalance(0.0)
    , currentBet(1.0)
    , isSpinning(false)
    , lastSpinTime(0)
    , symbolDist(0, static_cast<int>(SlotSymbol::SCATTER))
{
    // Initialize RNG with high-quality seed
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    rng.seed(seed);
    
    // Initialize stats
    stats = {0.0, 0.0, 0, 0, 96.5}; // 96.5% RTP target
    
    // Initialize paytable (simplified)
    paytable.resize(static_cast<int>(SlotSymbol::SCATTER) + 1);
    
    // Cherry payouts
    paytable[static_cast<int>(SlotSymbol::CHERRY)] = {0, 2, 5, 10, 25};
    paytable[static_cast<int>(SlotSymbol::LEMON)] = {0, 2, 5, 15, 30};
    paytable[static_cast<int>(SlotSymbol::ORANGE)] = {0, 3, 8, 20, 40};
    paytable[static_cast<int>(SlotSymbol::PLUM)] = {0, 3, 10, 25, 50};
    paytable[static_cast<int>(SlotSymbol::BELL)] = {0, 5, 15, 40, 100};
    paytable[static_cast<int>(SlotSymbol::BAR)] = {0, 10, 25, 75, 200};
    paytable[static_cast<int>(SlotSymbol::SEVEN)] = {0, 20, 50, 150, 500};
    paytable[static_cast<int>(SlotSymbol::DIAMOND)] = {0, 50, 100, 300, 1000};
    paytable[static_cast<int>(SlotSymbol::WILD)] = {0, 100, 250, 750, 2500};
    paytable[static_cast<int>(SlotSymbol::SCATTER)] = {0, 2, 5, 20, 100}; // Scatter pays anywhere
}

GameEngine::~GameEngine() {
    Shutdown();
}

bool GameEngine::Initialize() {
    // Reserve space for spin history (anti-cheat)
    spinHistory.reserve(1000);
    
    return true;
}

void GameEngine::Update() {
    // Update game logic, animations, etc.
    // This would be called every frame
}

void GameEngine::Shutdown() {
    // Cleanup resources
    spinHistory.clear();
}

SpinResult GameEngine::Spin(double betAmount) {
    SpinResult result;
    result.isWin = false;
    result.winAmount = 0.0;
    result.isBonusTriggered = false;
    result.isJackpot = false;
    
    // Security checks
    if (!CanSpin() || !ValidateSpinTiming() || DetectPatternAbuse()) {
        return result; // Return empty result on security failure
    }
    
    // Check balance
    if (currentBalance < betAmount) {
        return result;
    }
    
    // Deduct bet from balance
    currentBalance -= betAmount;
    currentBet = betAmount;
    
    // Generate reels
    result.reels.resize(REEL_COUNT);
    for (int i = 0; i < REEL_COUNT; i++) {
        result.reels[i] = GenerateReel();
    }
    
    // Calculate wins
    result = CalculateWin(result.reels);
    
    // Add winnings to balance
    currentBalance += result.winAmount;
    
    // Update statistics
    stats.totalBet += betAmount;
    stats.totalWin += result.winAmount;
    stats.totalSpins++;
    if (result.isWin) {
        stats.winningSpins++;
    }
    
    // Update RTP
    if (stats.totalBet > 0) {
        stats.rtp = (stats.totalWin / stats.totalBet) * 100.0;
    }
    
    // Update security tracking
    UpdateSpinHistory();
    
    return result;
}

std::vector<SlotSymbol> GameEngine::GenerateReel() {
    std::vector<SlotSymbol> reel(SYMBOL_COUNT);
    
    for (int i = 0; i < SYMBOL_COUNT; i++) {
        // Weighted symbol generation for realistic slot behavior
        int randomValue = rng() % 1000;
        
        if (randomValue < 200) {
            reel[i] = SlotSymbol::CHERRY;
        } else if (randomValue < 350) {
            reel[i] = SlotSymbol::LEMON;
        } else if (randomValue < 480) {
            reel[i] = SlotSymbol::ORANGE;
        } else if (randomValue < 600) {
            reel[i] = SlotSymbol::PLUM;
        } else if (randomValue < 720) {
            reel[i] = SlotSymbol::BELL;
        } else if (randomValue < 820) {
            reel[i] = SlotSymbol::BAR;
        } else if (randomValue < 900) {
            reel[i] = SlotSymbol::SEVEN;
        } else if (randomValue < 960) {
            reel[i] = SlotSymbol::DIAMOND;
        } else if (randomValue < 990) {
            reel[i] = SlotSymbol::WILD;
        } else {
            reel[i] = SlotSymbol::SCATTER;
        }
    }
    
    return reel;
}

SpinResult GameEngine::CalculateWin(const std::vector<std::vector<SlotSymbol>>& reels) {
    SpinResult result;
    result.reels = reels;
    result.winAmount = 0.0;
    result.isWin = false;
    
    // Check all paylines
    for (int line = 0; line < PAYLINES; line++) {
        if (CheckWinLine(reels, line)) {
            result.winLines.push_back(line);
            // Calculate line win (simplified)
            double lineWin = currentBet * 0.1; // Base win
            result.winAmount += lineWin;
            result.isWin = true;
        }
    }
    
    // Check for scatter wins
    int scatterCount = 0;
    for (const auto& reel : reels) {
        for (const auto& symbol : reel) {
            if (symbol == SlotSymbol::SCATTER) {
                scatterCount++;
            }
        }
    }
    
    if (scatterCount >= 3) {
        result.isBonusTriggered = true;
        result.winAmount += currentBet * scatterCount * 2;
        result.isWin = true;
    }
    
    // Check for jackpot (5 diamonds on payline)
    // This is simplified - real implementation would be more complex
    if (result.winAmount > currentBet * 100) {
        result.isJackpot = true;
    }
    
    return result;
}

bool GameEngine::CheckWinLine(const std::vector<std::vector<SlotSymbol>>& reels, int lineIndex) {
    // Simplified payline check - in reality, paylines have specific patterns
    // This checks the middle row for matching symbols
    if (reels.size() < 3) return false;
    
    SlotSymbol firstSymbol = reels[0][1]; // Middle symbol of first reel
    int matchCount = 1;
    
    for (int i = 1; i < std::min(static_cast<int>(reels.size()), 5); i++) {
        SlotSymbol currentSymbol = reels[i][1];
        if (currentSymbol == firstSymbol || currentSymbol == SlotSymbol::WILD || firstSymbol == SlotSymbol::WILD) {
            matchCount++;
        } else {
            break;
        }
    }
    
    return matchCount >= 3; // Need at least 3 matching symbols
}

bool GameEngine::CanSpin() const {
    return !isSpinning && currentBalance >= currentBet;
}

bool GameEngine::ValidateSpinTiming() {
    auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    
    // Prevent spins faster than 100ms (anti-bot protection)
    if (currentTime - lastSpinTime < 100) {
        return false;
    }
    
    lastSpinTime = currentTime;
    return true;
}

bool GameEngine::DetectPatternAbuse() {
    if (spinHistory.size() < 10) return false;
    
    // Check for suspicious patterns in spin timing
    std::vector<uint64_t> intervals;
    for (size_t i = 1; i < spinHistory.size(); i++) {
        intervals.push_back(spinHistory[i] - spinHistory[i-1]);
    }
    
    // Check if intervals are too regular (bot detection)
    if (intervals.size() >= 5) {
        double avgInterval = 0;
        for (auto interval : intervals) {
            avgInterval += interval;
        }
        avgInterval /= intervals.size();
        
        int regularCount = 0;
        for (auto interval : intervals) {
            if (std::abs(static_cast<double>(interval) - avgInterval) < 10) {
                regularCount++;
            }
        }
        
        // If more than 80% of intervals are too regular, flag as suspicious
        if (regularCount > intervals.size() * 0.8) {
            return true;
        }
    }
    
    return false;
}

void GameEngine::UpdateSpinHistory() {
    auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    
    spinHistory.push_back(currentTime);
    
    // Keep only last 100 spins for analysis
    if (spinHistory.size() > 100) {
        spinHistory.erase(spinHistory.begin());
    }
}

void GameEngine::ResetStats() {
    stats = {0.0, 0.0, 0, 0, 96.5};
}

bool GameEngine::IsSecure() const {
    // Perform various security checks
    return !DetectPatternAbuse() && stats.rtp > 80.0 && stats.rtp < 120.0;
}

void GameEngine::ReportSuspiciousActivity() {
    // Log suspicious activity for review
    // In a real implementation, this would send data to a security service
}