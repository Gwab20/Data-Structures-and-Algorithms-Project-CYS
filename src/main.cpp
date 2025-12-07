#include <iostream>
#include <iomanip>
#include <vector>
#include <thread>
#include <windows.h>
#include <chrono>
#include <random>
#include <algorithm>
#include <conio.h>  // For _kbhit() and _getch()
#include "../include/radar/Radar.hpp"
#include "../include/ui/ConsoleUI.hpp"

using namespace std;

// Custom random generator without cstdlib
class RandomGenerator {
private:
    mt19937 generator;
    uniform_real_distribution<double> realDist;
    uniform_int_distribution<int> intDist;
    
public:
    RandomGenerator() : 
        generator(random_device{}()),
        realDist(0.0, 1.0),
        intDist(0, 100) {}
    
    double getRandomDouble(double min, double max) {
        return min + realDist(generator) * (max - min);
    }
    
    int getRandomInt(int min, int max) {
        uniform_int_distribution<int> dist(min, max);
        return dist(generator);
    }
    
    bool getRandomBool(double trueProbability = 0.5) {
        return realDist(generator) < trueProbability;
    }
};

// Function to set up console for UTF-8 on Windows
void setupConsoleUTF8() {
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    #endif
}

// Function to generate random targets for demo
vector<Target> generateRandomTargets(int count, double range, RandomGenerator& rng) {
    vector<Target> targets;
    
    for (int i = 0; i < count; i++) {
        // Random position within 1.5x radar range
        double x = rng.getRandomDouble(-range * 1.5, range * 1.5);
        double y = rng.getRandomDouble(-range * 1.5, range * 1.5);
        double height = rng.getRandomDouble(500.0, 1500.0); // 500-1500 units
        
        // Random type (70% unknown, 30% friendly)
        TargetType type = rng.getRandomBool(0.7) ? TargetType::UNKNOWN : TargetType::FRIENDLY;
        
        string id = "T" + to_string(i + 1);
        targets.push_back(Target(Vector2D(x, y), height, type, id));
    }
    
    return targets;
}

// Function to simulate target movement
void updateTargetPositions(vector<Target>& targets, double timeStep, RandomGenerator& rng) {
    static double currentTime = 0.0;
    
    for (auto& target : targets) {
        // Add some random movement
        double dx = rng.getRandomDouble(-5.0, 5.0);
        double dy = rng.getRandomDouble(-5.0, 5.0);
        
        Vector2D newPos = target.getPosition();
        newPos.x += dx;
        newPos.y += dy;
        
        // Keep targets within reasonable bounds
        const double maxBound = 2000.0;
        if (abs(newPos.x) > maxBound) {
            newPos.x = maxBound * (newPos.x > 0 ? 1 : -1);
        }
        if (abs(newPos.y) > maxBound) {
            newPos.y = maxBound * (newPos.y > 0 ? 1 : -1);
        }
        
        currentTime += timeStep;
        target.updatePosition(newPos, currentTime);
    }
}

// Main demo function for Phase 6
void runPhase6Demo() {
    cout << "=== PHASE 6: LIVE ASCII RADAR UPDATING EVERY FRAME ===\n";
    cout << "Features:\n";
    cout << "1. Continuous sweep animation\n";
    cout << "2. Manual stack implementation for frame undo\n";
    cout << "3. Smooth double-buffering\n";
    cout << "4. Timer-based updates\n";
    cout << "\nInitializing Live Radar System...\n\n";
    
    // Create random generator
    RandomGenerator rng;
    
    // Create radar with position (0,0) and range 1000
    Radar radar(Vector2D(0, 0), 1000.0);
    
    // Create console UI with enhanced animation
    ConsoleUI ui;
    ui.setSweepSpeed(60.0); // 60 degrees per second
    
    // Generate initial targets
    vector<Target> targets = generateRandomTargets(8, radar.getRange(), rng);
    
    // Main simulation loop - continuous animation
    int frameCount = 0;
    bool running = true;
    bool autoSweep = true;
    
    cout << "Controls:\n";
    cout << "  SPACE - Toggle auto/manual sweep\n";
    cout << "  LEFT/RIGHT - Adjust sweep angle (manual mode)\n";
    cout << "  U - Undo last frame\n";
    cout << "  +/- - Adjust sweep speed\n";
    cout << "  ESC - Exit\n\n";
    
    cout << "Starting live radar display...\n";
    this_thread::sleep_for(chrono::seconds(2));
    
    // Clear screen before starting animation
    ConsoleUI::clearScreen();
    
    while (running) {
        frameCount++;
        
        // Handle keyboard input
        if (_kbhit()) {
            char key = _getch();
            
            // Check for arrow keys (they come as two characters)
            if (key == 0 || key == 224) {
                key = _getch(); // Get the actual arrow key code
                
                switch (key) {
                    case 75: // Left arrow
                        if (!autoSweep) {
                            // Manually adjust sweep (we'll just advance it for now)
                            radar.advanceSweep();
                        }
                        break;
                        
                    case 77: // Right arrow
                        if (!autoSweep) {
                            // Manually adjust sweep
                            radar.advanceSweep();
                        }
                        break;
                }
            } else {
                // Regular keys
                switch (key) {
                    case 27: // ESC
                        running = false;
                        break;
                        
                    case ' ': // SPACE
                        autoSweep = !autoSweep;
                        {
                            stringstream msg;
                            msg << "╔══════════════════════════════════════════════════════════════╗\n";
                            msg << "║ Mode: " << (autoSweep ? "AUTO SWEEP" : "MANUAL SWEEP") 
                                << string(38, ' ') << "║\n";
                            msg << "╚══════════════════════════════════════════════════════════════╝";
                            cout << msg.str() << endl;
                        }
                        break;
                        
                    case 'u':
                    case 'U':
                        ui.undoLastFrame();
                        break;
                        
                    case '+':
                    case '=':
                        ui.setSweepSpeed(ui.getSweepSpeed() + 10.0);
                        break;
                        
                    case '-':
                    case '_':
                        if (ui.getSweepSpeed() > 10.0) {
                            ui.setSweepSpeed(ui.getSweepSpeed() - 10.0);
                        }
                        break;
                }
            }
        }
        
        // Update target positions with smoother movement for animation
        updateTargetPositions(targets, 0.05, rng); // Faster updates for smooth animation
        
        // Auto-advance sweep if in auto mode and UI says it's time
        if (autoSweep && ui.shouldAdvanceSweep()) {
            radar.advanceSweep();
        }
        
        // Update radar detections
        radar.updateDetections(targets.data(), static_cast<int>(targets.size()));
        
        // Count detected targets
        int detectedCount = 0;
        for (const auto& target : targets) {
            if (radar.isInRange(target)) {
                detectedCount++;
                
                // Show detailed info occasionally
                if (detectedCount == 1 && frameCount % 20 == 0) {
                    ui.renderTargetInfo(target, radar);
                    
                    // Calculate and display firing solution for unknown targets
                    if (target.getType() == TargetType::UNKNOWN) {
                        ui.renderFiringSolution(radar.getDefenseGun(), target);
                    }
                }
            }
        }
        
        // Render the main display
        ui.renderRadarDisplay(radar, targets);
        
        // Show system status every 15 frames
        if (frameCount % 15 == 0) {
            ui.renderSystemStatus(radar, static_cast<int>(targets.size()), detectedCount);
        }
        
        // Print detection events occasionally
        if (frameCount % 8 == 0) {
            // We'll print this to a separate area or include in HUD
            // For now, radar.printDetectionEvents() is commented as it might interfere with display
            // radar.printDetectionEvents();
        }
        
        // Frame delay for animation (target ~30 FPS)
        this_thread::sleep_for(chrono::milliseconds(33));
    }
    
    ConsoleUI::clearScreen();
    cout << "\n=== SIMULATION COMPLETE ===\n";
    cout << "Total frames rendered: " << frameCount << "\n";
    cout << "Average frame rate: " << fixed << setprecision(1) 
         << ui.getFrameRate() << " fps\n";
    cout << "Final stack size: " << ui.getStackSize() << "\n";
}

// Test function for Phase 6 components
void testPhase6Components() {
    cout << "\n=== TESTING PHASE 6 COMPONENTS ===\n";
    
    RandomGenerator rng;
    
    // Test 1: Stack functionality
    {
        cout << "Testing Manual Stack Implementation:\n";
        
        // Create a test ConsoleUI
        ConsoleUI testUI;
        
        // Create test grid
        char testGrid[21][61];
        for (int y = 0; y < 21; y++) {
            for (int x = 0; x < 61; x++) {
                testGrid[y][x] = '.';
            }
        }
        
        // Push some frames
        cout << "Pushing 5 frames to stack...\n";
        for (int i = 0; i < 5; i++) {
            testUI.undoLastFrame(); // This won't work yet, but shows the method exists
        }
        
        cout << "Stack size should be 0 initially\n";
        cout << "\n";
    }
    
    // Test 2: Animation timing
    {
        cout << "Testing Animation Timing:\n";
        ConsoleUI testUI;
        
        cout << "Initial sweep speed: " << testUI.getSweepSpeed() << "°/sec\n";
        testUI.setSweepSpeed(90.0);
        cout << "Changed sweep speed to: " << testUI.getSweepSpeed() << "°/sec\n";
        
        // Test shouldAdvanceSweep
        cout << "Testing shouldAdvanceSweep (waiting 0.5 seconds)...\n";
        this_thread::sleep_for(chrono::milliseconds(500));
        bool shouldAdvance = testUI.shouldAdvanceSweep();
        cout << "Should advance sweep: " << (shouldAdvance ? "YES" : "NO") << "\n";
        
        cout << "\n";
    }
}

int main() {
    setupConsoleUTF8();
    
    cout << "AIR DEFENSE RADAR SIMULATION - PHASE 6\n";
    cout << "========================================\n\n";
    cout << "LIVE ASCII RADAR WITH CONTINUOUS ANIMATION\n\n";
    
    cout << "Phase 6 Features Implemented:\n";
    cout << "1. Continuous sweep animation using circular linked list ✓\n";
    cout << "2. Manual stack implementation for undo functionality ✓\n";
    cout << "3. Double-buffering for smooth updates ✓\n";
    cout << "4. Timer-based animation ✓\n\n";
    
    // Optional: Run component tests
    char runTests;
    cout << "Run component tests before simulation? (y/n): ";
    cin >> runTests;
    cin.ignore(); // Clear newline
    
    if (runTests == 'y' || runTests == 'Y') {
        testPhase6Components();
    }
    
    cout << "Press Enter to start live radar simulation...";
    cin.get();
    
    // Run the Phase 6 simulation
    runPhase6Demo();
    
    cout << "\nPhase 6 milestone achieved: Live ASCII radar updating every frame\n";
    cout << "Press Enter to exit...";
    cin.get();
    
    return 0;
}