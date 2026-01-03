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
#include <ctime>
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
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
        
        // ========== PHASE 7: ENABLE MOUSE INPUT ==========
        HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
        GetConsoleMode(hIn, &dwMode);
        dwMode |= ENABLE_MOUSE_INPUT;
        dwMode &= ~ENABLE_QUICK_EDIT_MODE;
        SetConsoleMode(hIn, dwMode);
        // ========== END PHASE 7 ==========
    #endif
}

// Show fancy controls screen
void clearScreen() {
    cout << "\033[2J\033[1;1H";
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
    cout << "║  C          - Toggle MOUSE control                           ║\n";
    cout << "║  (With mouse control enabled)                                ║\n";
    cout << "║  ARROWS     - Move mouse cursor                              ║\n";
    cout << "║  SPACE      - Create mouse-controlled target                 ║\n";
    cout << "║                                                              ║\n";
    cout << "║  [AUTO MODE]                                                 ║\n";
    cout << "║  + / =      - Increase sweep speed                           ║\n";
    cout << "║  - / _      - Decrease sweep speed                           ║\n";
    cout << "║  M          - Switch to MANUAL mode                          ║\n";
    cout << "║  C          - Toggle MOUSE control                           ║\n";
    cout << "║                                                              ║\n";
    cout << "║  [COMMON]                                                    ║\n";
    cout << "║  U          - Undo last frame                                ║\n";
    cout << "║  ESC        - Exit simulation                                ║\n";
    cout << "╠══════════════════════════════════════════════════════════════╣\n";
    cout << "║  LEGEND: X=Unknown/Enemy  F=Friendly  ╱╲│=Sweep Line         ║\n";
    cout << "║          O=Mouse-Controlled Target  M=Mouse Cursor           ║\n";    
    cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    cout << "Press ENTER to begin simulation... ";
}

// Create random flying targets
vector<Target> generateRandomTargets(int count, double range, RandomGenerator& rng) {
    vector<Target> targets;
    
    for (int i = 0; i < count; i++) {
        double x = rng.getRandomDouble(-range * 1.2, range * 1.2);
        double y = rng.getRandomDouble(-range * 1.2, range * 1.2);
        double height = rng.getRandomDouble(300.0, 2000.0);
        
        TargetType type = rng.getRandomBool(0.6) ? TargetType::UNKNOWN : TargetType::FRIENDLY;
        
        string id = "TGT-" + to_string(1000 + i);
        targets.push_back(Target(Vector2D(x, y), height, type, id));
    }
    
    return targets;
}

// Move targets around realistically (EXCLUDING mouse-controlled target)
void updateTargetPositions(vector<Target>& targets, double timeStep, RandomGenerator& rng, 
                          const string& excludeTargetId = "") {
    static double currentTime = 0.0;
    
    for (auto& target : targets) {
        // Skip mouse-controlled target
        if (target.getId() == excludeTargetId) {
            continue;
        }
        
        static map<string, Vector2D> movementPatterns;
        string id = target.getId();
        
        if (movementPatterns.find(id) == movementPatterns.end()) {
            movementPatterns[id] = Vector2D(
                rng.getRandomDouble(-3.0, 3.0),
                rng.getRandomDouble(-3.0, 3.0)
            );
        }
        
        Vector2D newPos = target.getPosition();
        newPos.x += movementPatterns[id].x;
        newPos.y += movementPatterns[id].y;
        
        const double maxBound = 1500.0;
        if (abs(newPos.x) > maxBound) {
            newPos.x = maxBound * (newPos.x > 0 ? 0.9 : -0.9);
            movementPatterns[id].x *= -1;
        }
        if (abs(newPos.y) > maxBound) {
            newPos.y = maxBound * (newPos.y > 0 ? 0.9 : -0.9);
            movementPatterns[id].y *= -1;
        }
        
        currentTime += timeStep;
        target.updatePosition(newPos, currentTime);
    }
}

// Keyboard functions
char waitForKey() {
    return _getch();
}

bool keyAvailable() {
    return _kbhit() != 0;
}

// Pretty headers
void printStatusHeader(bool autoMode, double sweepSpeed, int frameCount, bool mouseControl) {
    cout << "\033[36m";
    
    if (autoMode) {
        cout << "┌─[AUTO]────────────────────────────────────────────────────────┐\n";
        cout << "│ Sweep: " << setw(5) << fixed << setprecision(1) << sweepSpeed 
             << "°/sec │ Frame: " << setw(6) << frameCount 
             << " │ 'M'=Manual";
        if (mouseControl) cout << " │ MOUSE=ON";
        cout << " │\n";
    } else {
        cout << "┌─[MANUAL]──────────────────────────────────────────────────────┐\n";
        cout << "│ Frame: " << setw(6) << frameCount 
             << " │ SPACE=Next │ ←/→=Sweep │ 'A'=Auto";
        if (mouseControl) cout << " │ MOUSE=ON";
        cout << " │\n";
    }
    cout << "└──────────────────────────────────────────────────────────────┘\033[0m\n";
}

void printFiringSolutionHeader() {
    cout << "\033[31m";
    cout << "┌─────────────────────[ FIRING SOLUTIONS ]───────────────────────┐\033[0m\n";
}

void printTargetInfoHeader() {
    cout << "\033[33m";
    cout << "┌──────────────────────[ TARGET INFO ]─────────────────────────┐\033[0m\n";
}

void printSeparator() {
    cout << "\033[90m";
    cout << "├────────────────────────────────────────────────────────────────┤\033[0m\n";
}

void printFooter() {
    cout << "\033[90m";
    cout << "└────────────────────────────────────────────────────────────────┘\033[0m\n";
}

// Find mouse-controlled target
Target* findMouseTarget(vector<Target>& targets, const string& mouseTargetId) {
    for (auto& target : targets) {
        if (target.getId() == mouseTargetId) {
            return &target;
        }
    }
    return nullptr;
}

// main simulation loop
void runRadarSimulation() {
    RandomGenerator rng;
    Radar radar(Vector2D(0, 0), 1000.0);
    ConsoleUI ui;
    
    ui.setSweepSpeed(45.0);
    
    vector<Target> targets = generateRandomTargets(8, radar.getRange(), rng);
    
    int frameCount = 0;
    bool running = true;
    bool autoMode = false;
    
    map<string, int> targetSolutionCount;
    
    clearScreen();
    
    cout << "\033[36m";
    cout << "╔══════════════════════════════════════════════════════════════╗\n";
    cout << "║              RADAR SYSTEM INITIALIZED - READY                ║\n";
    cout << "╚══════════════════════════════════════════════════════════════╝\033[0m\n";
    cout << "\nStarting in MANUAL frame-by-frame mode...\n";
    cout << "Press SPACE for first frame\n";
    
    char startKey = waitForKey();
    if (startKey == 27) return;
    
    // Main simulation loop
    while (running) {
        clearScreen();
        
        printStatusHeader(autoMode, ui.getSweepSpeed(), frameCount, ui.isMouseControlEnabled());
        cout << "\n";
        
        // ========== INPUT HANDLING ==========
        char key = 0;
        
        if (!autoMode) {
            cout << "\033[33m[MANUAL MODE - Press key for next frame]\033[0m\n";
            cout << "SPACE:Next │ ←/→:Sweep │ A:Auto │ C:Mouse │ ESC:Exit\n\n";
            
            key = waitForKey();
            
            if (key == 27) { running = false; break; }
            if (key == 'a' || key == 'A') { autoMode = true; }
            if (key == 'c' || key == 'C') { 
                ui.toggleMouseControl(!ui.isMouseControlEnabled());
            }
            if (key == 'u' || key == 'U') { ui.undoLastFrame(); }
            
            // Handle arrow keys for mouse control or sweep control
            if (key == 0 || key == 224) {
                int arrowKey = _getch();
                
                if (ui.isMouseControlEnabled()) {
                    // Arrow keys control mouse cursor
                    switch(arrowKey) {
                        case 72: ui.moveMouseCursor(0, -2); break; // Up
                        case 80: ui.moveMouseCursor(0, 2); break;  // Down
                        case 75: ui.moveMouseCursor(-2, 0); break; // Left
                        case 77: ui.moveMouseCursor(2, 0); break;  // Right
                    }
                } else {
                    // Arrow keys control sweep
                    if (arrowKey == 75) {
                        radar.advanceSweep();
                        ui.displayMessage("Sweep: LEFT");
                    }
                    else if (arrowKey == 77) {
                        radar.advanceSweep();
                        ui.displayMessage("Sweep: RIGHT");
                    }
                }
            }
            
            // Only advance frame if valid key was pressed
            if (key != 'a' && key != 'A' && key != 'c' && key != 'C' && 
                key != 'u' && key != 'U') {
                frameCount++;
            } else if (key == 'a' || key == 'A') {
                continue;
            }
        } else {
            // Auto mode
            if (keyAvailable()) {
                key = _getch();
                
                if (key == 27) { running = false; break; }
                if (key == 'm' || key == 'M') { autoMode = false; }
                if (key == 'c' || key == 'C') { 
                    ui.toggleMouseControl(!ui.isMouseControlEnabled());
                }
                if (key == 'u' || key == 'U') { ui.undoLastFrame(); }
                
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
            
            if (ui.shouldAdvanceSweep()) {
                radar.advanceSweep();
            }
            
            frameCount++;
            this_thread::sleep_for(chrono::milliseconds(40));
        }
        
        // ========== SIMULATION UPDATES ==========
        updateTargetPositions(targets, 0.1, rng, "");
        radar.updateDetections(targets.data(), static_cast<int>(targets.size()));
        
        // ========== RENDER RADAR DISPLAY ==========
        // Note: targets passed by reference for mouse control
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
            
            int solutionsShown = 0;
            for (auto targetPtr : unknownTargets) {
                if (solutionsShown >= 3) break;
                
                FiringSolution solution = radar.getDefenseGun().calculateFiringSolution(*targetPtr);
                
                cout << "\033[97m";
                cout << "│ " << targetPtr->getId() << " │ ";
                cout << "Az: " << setw(6) << fixed << setprecision(1) << solution.azimuth << "° │ ";
                cout << "El: " << setw(5) << solution.elevation << "° │ ";
                cout << "Dist: " << setw(7) << fixed << setprecision(1) << solution.distance << "m │ ";
                cout << solution.direction << "\033[0m\n";
                
                solutionsShown++;
                targetSolutionCount[targetPtr->getId()]++;
            }
            printFooter();
            cout << "\n";
        }
        
        // ========== DISPLAY TARGET INFO ==========
        if (inRangeCount > 0 && frameCount % 8 == 0) {
            printTargetInfoHeader();
            
            for (auto& target : targets) {
                if (radar.isInRange(target)) {
                    double dist = target.calculateHorizontalDistance(radar.getPosition());
                    double bearing = target.calculateBearingFrom(radar.getPosition());
                    string dir = target.getCompassDirectionFrom(radar.getPosition());
                    double speed = target.getSpeed();
                    
                    cout << "\033[97m";
                    cout << "│ " << target.getId() << " │ ";
                    cout << "Type: " << (target.getType() == TargetType::UNKNOWN ? "HOSTILE " : "FRIENDLY") << " │ ";
                    cout << "Range: " << setw(6) << fixed << setprecision(1) << dist << "m │ ";
                    cout << "Brg: " << setw(5) << bearing << "° │ ";
                    cout << "Spd: " << setw(5) << speed << "m/s │ ";
                    cout << dir << "\033[0m\n";
                    
                    break;
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
             << " │ Friendly: " << friendlyTargets.size();
        
        if (ui.isMouseControlEnabled()) {
            cout << " │ 🖱️:ON";
        } else {
            cout << " │ 🖱️:OFF";
        }
        
        cout << " │ Sweep: " << setw(5) << fixed << setprecision(1) 
             << radar.getCurrentSweepAngle() << "° │\033[0m\n";
        printFooter();
        
        if (!autoMode) {
            cout << "\n\033[90m" << "Waiting for next command..." << "\033[0m\n";
        } else {
            cout << "\n\033[90m" << "Auto sweep active... " 
                 << "(Speed: " << fixed << setprecision(0) << ui.getSweepSpeed() 
                 << "°/sec)" << "\033[0m\n";
        }
        
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
    cout << "║  • Mouse Control: " << (ui.isMouseControlEnabled() ? "ACTIVE           " : "INACTIVE          ") << "                        ║\n";
    cout << "║  • Targets Tracked: " << setw(4) << targets.size() << "                                      ║\n";
    cout << "║                                                              ║\n";
    cout << "╚══════════════════════════════════════════════════════════════╝\033[0m\n\n";
    
    cout << "Press any key to exit...";
    waitForKey();
}

int main() {
    setupConsoleUTF8();
    
    displayControls();
    
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