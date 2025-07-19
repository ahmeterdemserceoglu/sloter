#include "GameUI.h"
#include <iostream>

GameUI::GameUI() 
    : screenWidth(1024)
    , screenHeight(768)
    , aspectRatio(4.0f/3.0f)
    , isMenuVisible(false)
    , isSettingsVisible(false)
    , isTouching(false)
{
    // Initialize UI elements
    buttons.reserve(10);
    labels.reserve(20);
    reels.reserve(5);
}

GameUI::~GameUI() {
    Shutdown();
}

bool GameUI::Initialize() {
    std::cout << "Initializing Game UI..." << std::endl;
    
    // Initialize graphics (simplified for console)
    InitializeGraphics();
    
    // Setup UI layout
    LayoutMainUI();
    
    std::cout << "Game UI initialized successfully" << std::endl;
    return true;
}

void GameUI::Shutdown() {
    std::cout << "Shutting down Game UI..." << std::endl;
    
    // Clear UI elements
    buttons.clear();
    labels.clear();
    reels.clear();
    
    // Clear textures (simplified)
    textures.clear();
}

void GameUI::Render() {
    // Simplified rendering for console
    // In a real implementation, this would use OpenGL
    
    // Clear screen (conceptually)
    // RenderBackground();
    
    // Render UI elements
    for (const auto& button : buttons) {
        // RenderButton(button);
    }
    
    for (const auto& label : labels) {
        // RenderLabel(label);
    }
    
    for (const auto& reel : reels) {
        // RenderSlotReel(reel);
    }
    
    // Render effects
    // RenderEffects();
}

void GameUI::Update(float deltaTime) {
    // Update animations
    UpdateAnimations(deltaTime);
    
    // Update reel spinning
    for (auto& reel : reels) {
        if (reel.isSpinning) {
            reel.currentOffset += reel.spinSpeed * deltaTime;
            
            // Stop spinning after some time (simplified)
            if (reel.currentOffset > 1000.0f) {
                reel.isSpinning = false;
                reel.currentOffset = 0.0f;
            }
        }
    }
}

void GameUI::InitializeGraphics() {
    // Simplified graphics initialization
    // In a real implementation, this would initialize OpenGL
    std::cout << "Graphics initialized (console mode)" << std::endl;
}

void GameUI::LayoutMainUI() {
    // Create main UI elements
    
    // Balance label
    balanceLabel.position = Vector2(50, 50);
    balanceLabel.text = "Balance: $0.00";
    balanceLabel.color = Color(1.0f, 1.0f, 1.0f, 1.0f);
    balanceLabel.fontSize = 24.0f;
    balanceLabel.isVisible = true;
    
    // Bet label
    betLabel.position = Vector2(50, 100);
    betLabel.text = "Bet: $1.00";
    betLabel.color = Color(1.0f, 1.0f, 1.0f, 1.0f);
    betLabel.fontSize = 20.0f;
    betLabel.isVisible = true;
    
    // Win label
    winLabel.position = Vector2(50, 150);
    winLabel.text = "Win: $0.00";
    winLabel.color = Color(1.0f, 1.0f, 0.0f, 1.0f);
    winLabel.fontSize = 20.0f;
    winLabel.isVisible = true;
    
    // Spin button
    spinButton.bounds = Rect(400, 600, 200, 60);
    spinButton.text = "SPIN";
    spinButton.backgroundColor = Color(0.0f, 0.8f, 0.0f, 1.0f);
    spinButton.textColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
    spinButton.isEnabled = true;
    spinButton.isPressed = false;
    
    // Max bet button
    maxBetButton.bounds = Rect(650, 600, 150, 60);
    maxBetButton.text = "MAX BET";
    maxBetButton.backgroundColor = Color(0.8f, 0.0f, 0.0f, 1.0f);
    maxBetButton.textColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
    maxBetButton.isEnabled = true;
    maxBetButton.isPressed = false;
    
    // Auto spin button
    autoSpinButton.bounds = Rect(850, 600, 150, 60);
    autoSpinButton.text = "AUTO";
    autoSpinButton.backgroundColor = Color(0.0f, 0.0f, 0.8f, 1.0f);
    autoSpinButton.textColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
    autoSpinButton.isEnabled = true;
    autoSpinButton.isPressed = false;
    
    // Create slot reels
    for (int i = 0; i < 5; i++) {
        SlotReel reel;
        reel.bounds = Rect(200 + i * 120, 200, 100, 300);
        reel.symbols = {0, 1, 2}; // Default symbols
        reel.spinSpeed = 0.0f;
        reel.currentOffset = 0.0f;
        reel.isSpinning = false;
        
        reels.push_back(reel);
    }
    
    // Add to collections
    buttons.push_back(spinButton);
    buttons.push_back(maxBetButton);
    buttons.push_back(autoSpinButton);
    
    labels.push_back(balanceLabel);
    labels.push_back(betLabel);
    labels.push_back(winLabel);
}

void GameUI::UpdateAnimations(float deltaTime) {
    // Update active animations
    for (auto it = activeAnimations.begin(); it != activeAnimations.end();) {
        auto& animation = *it;
        
        if (animation.isActive) {
            animation.currentTime += deltaTime;
            
            if (animation.currentTime >= animation.duration) {
                // Animation completed
                if (animation.completeFunc) {
                    animation.completeFunc();
                }
                animation.isActive = false;
                it = activeAnimations.erase(it);
            } else {
                // Update animation
                if (animation.updateFunc) {
                    float progress = animation.currentTime / animation.duration;
                    animation.updateFunc(progress);
                }
                ++it;
            }
        } else {
            it = activeAnimations.erase(it);
        }
    }
}

void GameUI::SetScreenSize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
    aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    
    // Update layout
    UpdateLayout();
}

void GameUI::UpdateLayout() {
    // Recalculate UI element positions based on screen size
    // This would be more complex in a real implementation
}

void GameUI::UpdateBalance(double balance) {
    balanceLabel.text = "Balance: $" + std::to_string(balance);
    std::cout << "Balance updated: $" << balance << std::endl;
}

void GameUI::UpdateBet(double bet) {
    betLabel.text = "Bet: $" + std::to_string(bet);
    std::cout << "Bet updated: $" << bet << std::endl;
}

void GameUI::UpdateWin(double winAmount) {
    winLabel.text = "Win: $" + std::to_string(winAmount);
    std::cout << "Win updated: $" << winAmount << std::endl;
    
    if (winAmount > 0) {
        ShowWinEffect(winAmount);
    }
}

void GameUI::UpdateReels(const std::vector<std::vector<int>>& reelData) {
    if (reelData.size() != reels.size()) {
        std::cout << "Warning: Reel data size mismatch" << std::endl;
        return;
    }
    
    for (size_t i = 0; i < reels.size() && i < reelData.size(); i++) {
        reels[i].symbols = reelData[i];
    }
    
    std::cout << "Reels updated" << std::endl;
}

void GameUI::StartSpinAnimation() {
    std::cout << "Starting spin animation..." << std::endl;
    
    // Start spinning all reels
    for (auto& reel : reels) {
        reel.isSpinning = true;
        reel.spinSpeed = 500.0f + (rand() % 200); // Random speed
    }
    
    // Create spin animation
    Animation spinAnim;
    spinAnim.duration = 3.0f; // 3 seconds
    spinAnim.currentTime = 0.0f;
    spinAnim.isActive = true;
    
    spinAnim.updateFunc = [this](float progress) {
        // Update spin effects
        for (auto& reel : reels) {
            if (progress > 0.8f) {
                // Start slowing down
                reel.spinSpeed *= 0.95f;
            }
        }
    };
    
    spinAnim.completeFunc = [this]() {
        // Stop all reels
        for (auto& reel : reels) {
            reel.isSpinning = false;
            reel.spinSpeed = 0.0f;
        }
        std::cout << "Spin animation completed" << std::endl;
    };
    
    activeAnimations.push_back(spinAnim);
}

void GameUI::ShowWinEffect(double amount) {
    std::cout << "Showing win effect for $" << amount << std::endl;
    
    // Create win animation
    Animation winAnim;
    winAnim.duration = 2.0f;
    winAnim.currentTime = 0.0f;
    winAnim.isActive = true;
    
    winAnim.updateFunc = [this, amount](float progress) {
        // Flash win label
        winLabel.color.a = 0.5f + 0.5f * sin(progress * 10.0f);
    };
    
    winAnim.completeFunc = [this]() {
        winLabel.color.a = 1.0f;
    };
    
    activeAnimations.push_back(winAnim);
}

void GameUI::OnTouchDown(float x, float y) {
    lastTouchPos = Vector2(x, y);
    isTouching = true;
    
    // Check button presses
    for (auto& button : buttons) {
        if (IsPointInRect(lastTouchPos, button.bounds)) {
            button.isPressed = true;
            StartButtonPressAnimation(button);
        }
    }
}

void GameUI::OnTouchUp(float x, float y) {
    Vector2 touchPos(x, y);
    isTouching = false;
    
    // Check button releases
    for (auto& button : buttons) {
        if (button.isPressed && IsPointInRect(touchPos, button.bounds)) {
            // Button clicked
            if (button.onClick) {
                button.onClick();
            }
            std::cout << "Button clicked: " << button.text << std::endl;
        }
        button.isPressed = false;
    }
}

bool GameUI::IsPointInRect(const Vector2& point, const Rect& rect) {
    return point.x >= rect.x && point.x <= rect.x + rect.width &&
           point.y >= rect.y && point.y <= rect.y + rect.height;
}

void GameUI::StartButtonPressAnimation(UIButton& button) {
    // Simple button press animation
    button.backgroundColor.r *= 0.8f;
    button.backgroundColor.g *= 0.8f;
    button.backgroundColor.b *= 0.8f;
}

void GameUI::SetSpinButtonEnabled(bool enabled) {
    spinButton.isEnabled = enabled;
    if (!enabled) {
        spinButton.backgroundColor = Color(0.5f, 0.5f, 0.5f, 1.0f);
    } else {
        spinButton.backgroundColor = Color(0.0f, 0.8f, 0.0f, 1.0f);
    }
}

void GameUI::PlaySpinEffect() {
    std::cout << "Playing spin sound effect..." << std::endl;
    StartSpinAnimation();
}

// Callback setters (simplified)
void GameUI::SetSpinCallback(std::function<void()> callback) {
    spinButton.onClick = callback;
}

void GameUI::SetBetChangeCallback(std::function<void(double)> callback) {
    // Store callback for bet change
}

void GameUI::SetMaxBetCallback(std::function<void()> callback) {
    maxBetButton.onClick = callback;
}

void GameUI::SetAutoSpinCallback(std::function<void(bool)> callback) {
    autoSpinButton.onClick = [callback]() {
        static bool autoSpinActive = false;
        autoSpinActive = !autoSpinActive;
        callback(autoSpinActive);
    };
}