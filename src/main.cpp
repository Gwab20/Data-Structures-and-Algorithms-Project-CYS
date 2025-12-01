// Replace main.cpp with this enhanced version
#include <iostream>
#include <iomanip>
#include <cmath>
#include "../include/radar/Radar.hpp"
#include "../include/radar/Target.hpp"

using namespace std;

void testPhase2Kinematics() {
    cout << "=== PHASE 2: TARGET KINEMATICS TEST ===" << endl;
    
    // Create a target
    Target target(Vector2D(0, 0), 100.0, TargetType::UNKNOWN, "T1");
    
    // Simulate movement over time
    double currentTime = 0.0;
    const double timeStep = 0.1; // 100ms between updates
    
    cout << fixed << setprecision(2);
    
    for (int i = 0; i < 10; i++) {
        currentTime += timeStep;
        
        // Move target in a pattern (simulating mouse movement)
        double x = i * 10.0;  // 10 units per step in x
        double y = i * 5.0;   // 5 units per step in y 
        
        Vector2D newPos(x, y);
        target.updatePosition(newPos, currentTime);
        
        // Display kinematics data
        cout << "Time: " << currentTime << "s | ";
        cout << "Position: (" << x << ", " << y << ") | ";
        cout << "Speed: " << target.getSpeed() << " units/s | ";
        cout << "Velocity: (" << target.getVelocity().x << ", " 
                  << target.getVelocity().y << ") | ";
        cout << "Acceleration: (" << target.getAcceleration().x << ", " 
                  << target.getAcceleration().y << ")" << endl;
    }
    
    // Test with Radar integration
    cout << "\n=== RADAR INTEGRATION TEST ===" << endl;
    Radar radar(Vector2D(0, 0), 1000.0);
    
    double horizontalDist, displacement, bearing;
    string direction;
    
    radar.analyzeTarget(target, horizontalDist, displacement, bearing, direction);
    
    cout << "Target Analysis:" << endl;
    cout << "Horizontal Distance: " << horizontalDist << endl;
    cout << "Displacement: " << displacement << endl;
    cout << "Bearing: " << bearing << "°" << endl;
    cout << "Direction: " << direction << endl;
    cout << "Current Speed: " << target.getSpeed() << " units/s" << endl;
}

void testPhase3WithProperStructures() {
    cout << "\n\n=== PHASE 3 WITH QUEUE & CIRCULAR LINKED LIST ===" << endl;
    
    Radar radar(Vector2D(0, 0), 1000.0);
    
    // Create test targets
    const int TOTAL_TARGETS = 4;
    Target testTargets[TOTAL_TARGETS];
    
    testTargets[0] = Target(Vector2D(100, 100), 500, TargetType::UNKNOWN, "T1");
    testTargets[1] = Target(Vector2D(1500, 1500), 500, TargetType::UNKNOWN, "T2");
    testTargets[2] = Target(Vector2D(200, 200), 400, TargetType::FRIENDLY, "F1");
    testTargets[3] = Target(Vector2D(1200, 1200), 600, TargetType::UNKNOWN, "T3");
    
    cout << "Simulating radar sweeps with queue and circular list:" << endl;
    
    // Simulate multiple sweeps
    for (int sweep = 0; sweep < 8; sweep++) {
        cout << "\nSweep " << sweep << ": ";
        
        // Move targets to simulate dynamic scenario
        if (sweep == 2) {
            testTargets[1].setPosition(Vector2D(500, 500)); // T2 enters range
        }
        if (sweep == 4) {
            testTargets[0].setPosition(Vector2D(1500, 1500)); // T1 leaves range
        }
        if (sweep == 6) {
            testTargets[3].setPosition(Vector2D(300, 300)); // T3 enters range
        }
        
        radar.updateDetections(testTargets, TOTAL_TARGETS);
        radar.printDetectionEvents();
    }
    
    cout << "\n=== Queue Behavior Test ===" << endl;
    cout << "Final sweep demonstrates FIFO queue behavior" << endl;
}

int main() {
    // Run Phase 2 test first
    testPhase2Kinematics();
    
    // Then run Phase 3 test
    testPhase3WithProperStructures();
    
    return 0;
}