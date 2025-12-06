#ifndef GUN_HPP
#define GUN_HPP

#include "../utils/MathUtils.hpp"
#include "Target.hpp"
#include <string>
#include <map>
using namespace std;

struct FiringSolution {
    double elevation;
    double azimuth;
    string direction;
    double distance;
    string solutionText;
};

class Gun {
private:
    Vector2D position;
    ExpressionTree elevationTree;
    ExpressionTree azimuthTree;
    
    // Dynamic array for storing multiple firing solutions (history)
    FiringSolution* solutionHistory;
    int historySize;
    int currentHistoryIndex;
    static const int MAX_HISTORY = 50;
    
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
    map<string, double> createVariableMap(const Target& target) const;
    double calculateElevationAngle(const Target& target) const;
    double calculateAzimuthAngle(const Target& target) const;
};

#endif