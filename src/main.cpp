#include <iostream>
#include <iomanip>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <algorithm>
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
        
        static double currentTime = 0.0;
        currentTime += timeStep;
        target.updatePosition(newPos, currentTime);
    }
}

// Main demo function for Phase 5
void runRadarSimulationDemo() {
    cout << "=== PHASE 5: TEXT-ONLY REAL-TIME TERMINAL OUTPUT ===\n";
    cout << "Initializing Air Defense Radar System...\n\n";
    
    // Create random generator
    RandomGenerator rng;
    
    // Create radar with position (0,0) and range 1000
    Radar radar(Vector2D(0, 0), 1000.0);
    
    // Create console UI
    ConsoleUI ui;
    
    // Generate initial targets
    vector<Target> targets = generateRandomTargets(8, radar.getRange(), rng);
    
    // Main simulation loop
    int frameCount = 0;
    const int TOTAL_FRAMES = 50; // Run for 50 frames
    
    while (frameCount < TOTAL_FRAMES) {
        frameCount++;
        
        // Update target positions
        updateTargetPositions(targets, 0.1, rng); // 0.1 second time step
        
        // Update radar detections
        radar.updateDetections(targets.data(), static_cast<int>(targets.size()));
        
        // Count detected targets
        int detectedCount = 0;
        for (const auto& target : targets) {
            if (radar.isInRange(target)) {
                detectedCount++;
                
                // For every 5th frame, show detailed info for first detected target
                if (detectedCount == 1 && frameCount % 5 == 0) {
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
        
        // Show system status every 10 frames
        if (frameCount % 10 == 0) {
            ui.renderSystemStatus(radar, static_cast<int>(targets.size()), detectedCount);
        }
        
        // Print detection events if any
        radar.printDetectionEvents();
        
        // Frame delay for real-time effect (5 FPS for demo)
        this_thread::sleep_for(chrono::milliseconds(200));
    }
    
    cout << "\n=== SIMULATION COMPLETE ===\n";
    cout << "Frames rendered: " << frameCount << "\n";
    cout << "Average frame rate: " << fixed << setprecision(1) 
         << ui.getFrameRate() << " fps\n";
}

// Test function for individual components
void testPhase5Components() {
    cout << "\n=== TESTING PHASE 5 COMPONENTS ===\n";
    
    RandomGenerator rng;
    
    // Test 1: Random generator
    {
        cout << "Testing Random Generator:\n";
        vector<double> randomDoubles;
        vector<int> randomInts;
        
        for (int i = 0; i < 5; i++) {
            randomDoubles.push_back(rng.getRandomDouble(0.0, 100.0));
            randomInts.push_back(rng.getRandomInt(0, 100));
        }
        
        cout << "Random doubles: ";
        for (double d : randomDoubles) cout << d << " ";
        cout << "\nRandom ints: ";
        for (int i : randomInts) cout << i << " ";
        cout << "\nRandom bools: ";
        for (int i = 0; i < 5; i++) cout << (rng.getRandomBool() ? "T" : "F") << " ";
        cout << "\n\n";
    }
    
    // Test 2: Target generation
    {
        cout << "Testing Target Generation:\n";
        vector<Target> targets = generateRandomTargets(3, 1000.0, rng);
        
        for (const auto& target : targets) {
            cout << "Target " << target.getId() << ": ";
            cout << "Pos(" << target.getPosition().x << ", " << target.getPosition().y << ") ";
            cout << "Height: " << target.getHeight() << " ";
            cout << "Type: " << (target.getType() == TargetType::UNKNOWN ? "Unknown" : "Friendly");
            cout << "\n";
        }
        cout << "\n";
    }
    
    // Test 3: ConsoleUI formatting
    {
        ConsoleUI ui;
        cout << "Testing ConsoleUI Formatting:\n";
        
        double testValue = 123.456789;
        string formatted = ConsoleUI::formatDouble(testValue, 2);
        cout << "Format 123.456789 to 2 decimals: " << formatted 
             << " (expected: 123.46)\n";
        
        testValue = 0.12345;
        formatted = ConsoleUI::formatDouble(testValue, 3);
        cout << "Format 0.12345 to 3 decimals: " << formatted 
             << " (expected: 0.123)\n";
        
        cout << "\n";
    }
}

int main() {
    setupConsoleUTF8();
    
    cout << "AIR DEFENSE RADAR SIMULATION - PHASE 5\n";
    cout << "=======================================\n\n";
    
    // Optional: Run component tests
    testPhase5Components();
    
    cout << "Press Enter to start simulation...";
    cin.get();
    
    // Run the main simulation
    runRadarSimulationDemo();
    
    cout << "\nPhase 5 milestone achieved: Text-only real-time terminal output\n";
    cout << "Press Enter to exit...";
    cin.get();
    
    return 0;
}