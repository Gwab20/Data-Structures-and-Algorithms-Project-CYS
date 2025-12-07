#include <iostream>
#include <iomanip>
#include <vector>
#include <thread>
#include <windows.h>
#include <chrono>
#include <random>
#include <algorithm>
#include <conio.h>
#include <sstream>
#include "../include/radar/Radar.hpp"
#include "../include/ui/ConsoleUI.hpp"

using namespace std;

// Custom random generator
class RandomGenerator {
private:
    mt19937 generator;
    uniform_real_distribution<double> realDist;
    
public:
    RandomGenerator() : 
        generator(random_device{}()),
        realDist(0.0, 1.0) {}
    
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

// Setup Windows console for colors and UTF8
void setupConsoleUTF8() {
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8); // Enable UTF8
        SetConsoleCP(CP_UTF8);
        
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING; // Enable colors
        SetConsoleMode(hOut, dwMode);
    #endif
}

// Show fancy controls screen
void clearScreen() {
    cout << "\033[2J\033[1;1H"; // ANSI clear + home
}

void displayControls() {
    clearScreen();
    
    cout << "╔══════════════════════════════════════════════════════════════╗\n";
    cout << "║              AIR DEFENSE RADAR SIMULATION v1.0               ║\n";
    cout << "╠══════════════════════════════════════════════════════════════╣\n";
    cout << "║                                                              ║\n";
    cout << "║  MODES:                                                      ║\n";
    cout << "║  • MANUAL: Frame-by-frame control                            ║\n";
    cout << "║  • AUTO:   Continuous sweep animation                        ║\n";
    cout << "║                                                              ║\n";
    cout << "╠══════════════════════════════════════════════════════════════╣\n";
    cout << "║                      CONTROL MAPPINGS                        ║\n";
    cout << "╠══════════════════════════════════════════════════════════════╣\n";
    cout << "║  [MANUAL MODE]                                               ║\n";
    cout << "║  SPACE      - Advance one frame                              ║\n";
    cout << "║  ← / →      - Move sweep line                                ║\n";
    cout << "║  A          - Switch to AUTO mode                            ║\n";
    cout << "║                                                              ║\n";
    cout << "║  [AUTO MODE]                                                 ║\n";
    cout << "║  + / =      - Increase sweep speed                           ║\n";
    cout << "║  - / _      - Decrease sweep speed                           ║\n";
    cout << "║  M          - Switch to MANUAL mode                          ║\n";
    cout << "║                                                              ║\n";
    cout << "║  [COMMON]                                                    ║\n";
    cout << "║  U          - Undo last frame                                ║\n";
    cout << "║  ESC        - Exit simulation                                ║\n";
    cout << "╠══════════════════════════════════════════════════════════════╣\n";
    cout << "║  LEGEND: X=Unknown/Enemy  F=Friendly  ╱╲│=Sweep Line         ║\n";
    cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    cout << "Press ENTER to begin simulation... ";
}

// Create random flying targets
vector<Target> generateRandomTargets(int count, double range, RandomGenerator& rng) {
    vector<Target> targets;
    
    for (int i = 0; i < count; i++) {
        double x = rng.getRandomDouble(-range * 1.2, range * 1.2);
        double y = rng.getRandomDouble(-range * 1.2, range * 1.2);
        double height = rng.getRandomDouble(300.0, 2000.0); // flying high
        
        // 60% chance tp be enemy, 40% friendly
        TargetType type = rng.getRandomBool(0.6) ? TargetType::UNKNOWN : TargetType::FRIENDLY;
        
        string id = "TGT-" + to_string(1000 + i);
        targets.push_back(Target(Vector2D(x, y), height, type, id));
    }
    
    return targets;
}

// Move targets around realistically
void updateTargetPositions(vector<Target>& targets, double timeStep, RandomGenerator& rng) {
    static double currentTime = 0.0;
    
    for (auto& target : targets) {
        // Add some realistic movement patterns
        static map<string, Vector2D> movementPatterns;
        string id = target.getId();
        
        if (movementPatterns.find(id) == movementPatterns.end()) {
            // Create unique movement pattern for each target
            movementPatterns[id] = Vector2D(
                rng.getRandomDouble(-3.0, 3.0),  // X speed
                rng.getRandomDouble(-3.0, 3.0)    // Y speed
            );
        }
        
        Vector2D newPos = target.getPosition();
        newPos.x += movementPatterns[id].x;
        newPos.y += movementPatterns[id].y;
        
        // Keep targets within reasonable bounds
        const double maxBound = 1500.0;
        if (abs(newPos.x) > maxBound) {
            newPos.x = maxBound * (newPos.x > 0 ? 0.9 : -0.9);
            movementPatterns[id].x *= -1; // Bounce off boundary
        }
        if (abs(newPos.y) > maxBound) {
            newPos.y = maxBound * (newPos.y > 0 ? 0.9 : -0.9);
            movementPatterns[id].y *= -1; // Bounce off boundary
        }
        
        currentTime += timeStep;
        target.updatePosition(newPos, currentTime);
    }
}

// Keyboard functions for Windows
char waitForKey() {
    return _getch();
}

bool keyAvailable() {
    return _kbhit() != 0;
}

// Pretty headers for different sections
void printStatusHeader(bool autoMode, double sweepSpeed, int frameCount) {
    cout << "\033[36m"; // Cyan color
    
    if (autoMode) {
        cout << "┌─[AUTO]────────────────────────────────────────────────────────┐\n";
        cout << "│ Sweep: " << setw(5) << fixed << setprecision(1) << sweepSpeed 
             << "°/sec │ Frame: " << setw(6) << frameCount 
             << " │ Press 'M' for Manual │\n";
    } else {
        cout << "┌─[MANUAL]──────────────────────────────────────────────────────┐\n";
        cout << "│ Frame: " << setw(6) << frameCount 
             << " │ SPACE=Next │ ←/→=Sweep │ 'A'=Auto │\n";
    }
    cout << "└──────────────────────────────────────────────────────────────┘\033[0m\n";
}

void printFiringSolutionHeader() {
    cout << "\033[31m"; // Red color
    cout << "┌─────────────────────[ FIRING SOLUTIONS ]───────────────────────┐\033[0m\n";
}

void printTargetInfoHeader() {
    cout << "\033[33m"; // Yellow color
    cout << "┌──────────────────────[ TARGET INFO ]─────────────────────────┐\033[0m\n";
}

void printSeparator() {
    cout << "\033[90m"; // Gray color
    cout << "├────────────────────────────────────────────────────────────────┤\033[0m\n";
}

void printFooter() {
    cout << "\033[90m"; // Gray color
    cout << "└────────────────────────────────────────────────────────────────┘\033[0m\n";
}

// main simulation loop
void runRadarSimulation() {
    RandomGenerator rng;
    Radar radar(Vector2D(0, 0), 1000.0); // Radar at center, 1000m range
    ConsoleUI ui;
    
    // Set initial sweep speed
    ui.setSweepSpeed(45.0);
    
    // Generate targets
    vector<Target> targets = generateRandomTargets(8, radar.getRange(), rng);
    
    int frameCount = 0;
    bool running = true;
    bool autoMode = false; // Start in MANUAL mode
    
    // Track which targets we've shown solutions for
    map<string, int> targetSolutionCount;
    
    clearScreen();
    
    // Initial display
    cout << "\033[36m";
    cout << "╔══════════════════════════════════════════════════════════════╗\n";
    cout << "║              RADAR SYSTEM INITIALIZED - READY                ║\n";
    cout << "╚══════════════════════════════════════════════════════════════╝\033[0m\n";
    cout << "\nStarting in MANUAL frame-by-frame mode...\n";
    cout << "Press SPACE for first frame\n";
    
    // Wait for first key
    char startKey = waitForKey();
    if (startKey == 27) return; //ESC
    
    // Main simulation loop
    while (running) {
        clearScreen();
        
        // Print status header
        printStatusHeader(autoMode, ui.getSweepSpeed(), frameCount);
        cout << "\n";
        
        // ========== INPUT HANDLING ==========
        char key = 0;
        
        if (!autoMode) {
            // Manual mode - wait for key press
            cout << "\033[33m[MANUAL MODE - Press key for next frame]\033[0m\n";
            cout << "SPACE:Next │ ←/→:Sweep │ A:Auto │ ESC:Exit\n\n";
            
            key = waitForKey();
            
            if (key == 27) { running = false; break; }
            if (key == 'a' || key == 'A') { autoMode = true; }
            if (key == 'u' || key == 'U') { ui.undoLastFrame(); }
            
            // Handle arrow keys
            if (key == 0 || key == 224) {
                int arrowKey = _getch();
                if (arrowKey == 75) {  // LEFT
                    radar.advanceSweep();
                    ui.displayMessage("Sweep: LEFT");
                }
                else if (arrowKey == 77) {  // RIGHT
                    radar.advanceSweep();
                    ui.displayMessage("Sweep: RIGHT");
                }
            }
            
            // Only advance frame if valid key was pressed
            if (key != 'a' && key != 'A' && key != 'u' && key != 'U') {
                frameCount++;
            } else if (key == 'a' || key == 'A') {
                continue; // Skip to next iteration after mode change
            }
        } else {
            // Auto mode - check for keys without blocking
            if (keyAvailable()) {
                key = _getch();
                
                if (key == 27) { running = false; break; }
                if (key == 'm' || key == 'M') { autoMode = false; }
                if (key == 'u' || key == 'U') { ui.undoLastFrame(); }
                
                // Speed controls
                if (key == '+' || key == '=') {
                    ui.setSweepSpeed(ui.getSweepSpeed() + 15.0);
                    ui.displayMessage("Speed: +15°/sec");
                }
                else if (key == '-' || key == '_') {
                    if (ui.getSweepSpeed() > 15.0) {
                        ui.setSweepSpeed(ui.getSweepSpeed() - 15.0);
                        ui.displayMessage("Speed: -15°/sec");
                    }
                }
            }
            
            // Auto advance sweep
            if (ui.shouldAdvanceSweep()) {
                radar.advanceSweep();
            }
            
            frameCount++;
            this_thread::sleep_for(chrono::milliseconds(40)); // ~25 FPS
        }
        
        // ========== SIMULATION UPDATES ==========
        updateTargetPositions(targets, 0.1, rng);
        radar.updateDetections(targets.data(), static_cast<int>(targets.size()));
        
        // ========== RENDER RADAR DISPLAY ==========
        ui.renderRadarDisplay(radar, targets, autoMode);
        cout << "\n";
        
        // ========== TARGET PROCESSING ==========
        vector<Target*> unknownTargets;
        vector<Target*> friendlyTargets;
        int inRangeCount = 0;
        
        for (auto& target : targets) {
            if (radar.isInRange(target)) {
                inRangeCount++;
                if (target.getType() == TargetType::UNKNOWN) {
                    unknownTargets.push_back(&target);
                } else {
                    friendlyTargets.push_back(&target);
                }
            }
        }
        
        // ========== DISPLAY FIRING SOLUTIONS ==========
        if (!unknownTargets.empty()) {
            printFiringSolutionHeader();
            
            // Show solutions for up to 3 unknown targets
            int solutionsShown = 0;
            for (auto targetPtr : unknownTargets) {
                if (solutionsShown >= 3) break;
                
                FiringSolution solution = radar.getDefenseGun().calculateFiringSolution(*targetPtr);
                
                cout << "\033[97m"; // Bright white
                cout << "│ " << targetPtr->getId() << " │ ";
                cout << "Az: " << setw(6) << fixed << setprecision(1) << solution.azimuth << "° │ ";
                cout << "El: " << setw(5) << solution.elevation << "° │ ";
                cout << "Dist: " << setw(7) << fixed << setprecision(1) << solution.distance << "m │ ";
                cout << solution.direction << "\033[0m\n";
                
                solutionsShown++;
                
                // Track how many times we've shown this target
                targetSolutionCount[targetPtr->getId()]++;
            }
            printFooter();
            cout << "\n";
        }
        
        // ========== DISPLAY TARGET INFO ==========
        if (inRangeCount > 0 && frameCount % 8 == 0) {
            printTargetInfoHeader();
            
            // Show info for first target in range
            for (auto& target : targets) {
                if (radar.isInRange(target)) {
                    double dist = target.calculateHorizontalDistance(radar.getPosition());
                    double bearing = target.calculateBearingFrom(radar.getPosition());
                    string dir = target.getCompassDirectionFrom(radar.getPosition());
                    double speed = target.getSpeed();
                    
                    cout << "\033[97m"; // Bright white
                    cout << "│ " << target.getId() << " │ ";
                    cout << "Type: " << (target.getType() == TargetType::UNKNOWN ? "HOSTILE " : "FRIENDLY") << " │ ";
                    cout << "Range: " << setw(6) << fixed << setprecision(1) << dist << "m │ ";
                    cout << "Brg: " << setw(5) << bearing << "° │ ";
                    cout << "Spd: " << setw(5) << speed << "m/s │ ";
                    cout << dir << "\033[0m\n";
                    
                    break; // Only show first target
                }
            }
            printFooter();
            cout << "\n";
        }
        
        // ========== SYSTEM STATISTICS ==========
        printSeparator();
        cout << "\033[36m";
        cout << "│ Targets: " << setw(2) << inRangeCount << "/" << targets.size() 
             << " in range │ Unknown: " << unknownTargets.size() 
             << " │ Friendly: " << friendlyTargets.size() 
             << " │ Sweep: " << setw(5) << fixed << setprecision(1) 
             << radar.getCurrentSweepAngle() << "° │\033[0m\n";
        printFooter();
        
        // ========== MODE SPECIFIC MESSAGES ==========
        if (!autoMode) {
            cout << "\n\033[90m" << "Waiting for next command..." << "\033[0m\n";
        } else {
            cout << "\n\033[90m" << "Auto sweep active... " 
                 << "(Speed: " << fixed << setprecision(0) << ui.getSweepSpeed() 
                 << "°/sec)" << "\033[0m\n";
        }
        
        // Brief pause for readability in auto mode
        if (autoMode) {
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }
    
    // ========== EXIT SEQUENCE ==========
    clearScreen();
    cout << "\033[36m";
    cout << "╔══════════════════════════════════════════════════════════════╗\n";
    cout << "║                   SIMULATION COMPLETE                        ║\n";
    cout << "╠══════════════════════════════════════════════════════════════╣\n";
    cout << "║                                                              ║\n";
    cout << "║  Statistics:                                                 ║\n";
    cout << "║  • Total Frames: " << setw(8) << frameCount << "                                    ║\n";
    cout << "║  • Final Mode:   " << (autoMode ? "AUTO              " : "MANUAL            ") << "                          ║\n";
    cout << "║  • Targets Tracked: " << setw(4) << targets.size() << "                                      ║\n";
    cout << "║                                                              ║\n";
    cout << "╚══════════════════════════════════════════════════════════════╝\033[0m\n\n";
    
    cout << "Press any key to exit...";
    waitForKey();
}

int main() {
    setupConsoleUTF8();
    
    displayControls();
    
    // Wait for Enter
    while (true) {
        if (_kbhit()) {
            int key = _getch();
            
            if (key == 0 || key == 224) {
                _getch();
                continue;
            }
            
            if (key == 13 || key == 10) {
                break;
            } else if (key == 27) {
                clearScreen();
                cout << "Simulation terminated.\n";
                return 0;
            }
        }
    }
    
    runRadarSimulation();
    
    return 0;
}