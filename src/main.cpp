#include <iostream>
#include <iomanip>
#include "../include/radar/Radar.hpp"
#include "../include/radar/Target.hpp"

using namespace std;

void testPhase1() {
    cout << "=== PHASE 1 TEST: Distance, Direction, and Angle Conversions ===" << endl;
    
    // Create radar at origin (0,0) with 1000m range
    Radar radar(Vector2D(0, 0), 1000.0);
    
    // Test targets at different positions
    Target targets[] = {
        Target(Vector2D(100, 100), 500, TargetType::UNKNOWN, "T1"),  // NE quadrant
        Target(Vector2D(-150, 200), 300, TargetType::FRIENDLY, "T2"), // NW quadrant  
        Target(Vector2D(300, -50), 400, TargetType::UNKNOWN, "T3"),  // SE quadrant
        Target(Vector2D(-100, -200), 600, TargetType::UNKNOWN, "T4") // SW quadrant
    };
    
    cout << fixed << setprecision(2);
    
    for (const auto& target : targets) {
        double horizontalDist, displacement, bearing;
        string direction;
        
        // Analyze target
        radar.analyzeTarget(target, horizontalDist, displacement, bearing, direction);
        
        // Calculate firing solution
        FiringSolution solution = radar.getDefenseGun().calculateFiringSolution(target);
        
        cout << "\n--- Target " << target.getId() << " ---" << endl;
        cout << "Position: (" << target.getPosition().x << ", " << target.getPosition().y << ")" << endl;
        cout << "Height: " << target.getHeight() << "m" << endl;
        cout << "Horizontal Distance: " << horizontalDist << "m" << endl;
        cout << "Displacement: " << displacement << "m" << endl;
        cout << "Bearing: " << bearing << "°" << endl;
        cout << "Direction: " << direction << endl;
        cout << "In Radar Range: " << (radar.isInRange(target) ? "YES" : "NO") << endl;
        cout << "Firing Solution: " << solution.elevation << "° " << solution.direction << endl;
    }
    
    // Test edge cases
    cout << "\n--- Edge Case Tests ---" << endl;
    
    // Target directly north
    Target northTarget(Vector2D(0, 500), 300);
    auto northSolution = radar.getDefenseGun().calculateFiringSolution(northTarget);
    cout << "North Target - Direction: " << northSolution.direction 
              << ", Elevation: " << northSolution.elevation << "°" << endl;
    
    // Target directly east  
    Target eastTarget(Vector2D(500, 0), 300);
    auto eastSolution = radar.getDefenseGun().calculateFiringSolution(eastTarget);
    cout << "East Target - Direction: " << eastSolution.direction 
              << ", Elevation: " << eastSolution.elevation << "°" << endl;
}

int main() {
    testPhase1();
    return 0;
}