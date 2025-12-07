#ifndef GUN_HPP
#define GUN_HPP

#include "../utils/MathUtils.hpp"
#include "Target.hpp"
#include <string>
#include <map>
using namespace std;

// Stores all data needed to aim and fire at a target
struct FiringSolution {
    double elevation; // Up/down angle of gun
    double azimuth; // Left/right angle
    string direction; // Like "NE" or "SW"
    double distance;  // How far to target
    string solutionText; // Ready-to-display text
};

// Represents one artillery gun that can aim at targets
class Gun {
private:
    Vector2D position;  // Where the gun is on map
    ExpressionTree elevationTree; // Math formula for up/down angle
    ExpressionTree azimuthTree;  // Math formula for left/right angle
    
    // Dynamic array for storing multiple firing solutions (history)
    FiringSolution* solutionHistory;  // List of past shots
    int historySize;  // How many shots we've stored
    int currentHistoryIndex;  // Where to add next shot
    static const int MAX_HISTORY = 50;  // Max shots to remember
    
public:
    Gun(const Vector2D& pos = Vector2D(0,0));
    ~Gun();
    
    // Prevent copying for simplicity (or implement proper copy constructors)
    Gun(const Gun&) = delete;
    Gun& operator=(const Gun&) = delete;
    
    // Getters
    Vector2D getPosition() const { return position; }
    
    // Calculate firing solution - MADE CONST
    FiringSolution calculateFiringSolution(const Target& target) const;
    
    // Store solution in history
    void storeSolution(const FiringSolution& solution);
    
    // Get last N solutions
    void getRecentSolutions(FiringSolution* output, int n) const;
    
    // Format solution
    string formatFiringSolution(double elevation, double azimuth, const string& direction) const;
    
private:
 // Create data for the math formulas (distance, positions, etc.)
    map<string, double> createVariableMap(const Target& target) const;
    // Calculate up/down angle using math formula
    double calculateElevationAngle(const Target& target) const;
    // Calculate left/right angle using math formula
    double calculateAzimuthAngle(const Target& target) const;
};

#endif // GUN_HPP