#include "../../include/radar/Gun.hpp"
#include <sstream>
#include <iomanip>
#include <cstring> // For memcpy if needed

using namespace std;

Gun::Gun(const Vector2D& pos) 
    : position(pos), 
      elevationTree(ExpressionTree::createElevationExpression()),
      azimuthTree(ExpressionTree::createAzimuthExpression()),
      historySize(MAX_HISTORY),
      currentHistoryIndex(0) {
    
    // Allocate dynamic array for history
    solutionHistory = new FiringSolution[historySize];
}

Gun::~Gun() {
    // Clean up dynamic array
    delete[] solutionHistory;
}

map<string, double> Gun::createVariableMap(const Target& target) const {
    map<string, double> variables;
    
    double horizontalDist = target.calculateHorizontalDistance(position);
    double height = target.getHeight();
    Vector2D targetPos = target.getPosition();
    
    variables["height"] = height;
    variables["distance"] = horizontalDist;
    variables["dx"] = targetPos.x - position.x;
    variables["dy"] = targetPos.y - position.y;
    variables["pi"] = M_PI;
    
    return variables;
}

double Gun::calculateElevationAngle(const Target& target) const {
    auto variables = createVariableMap(target);
    double elevation = elevationTree.evaluate(variables);
    
    // Validate range
    if (elevation < 0) elevation = 0;
    if (elevation > 90) elevation = 90;
    
    return elevation;
}

double Gun::calculateAzimuthAngle(const Target& target) const {
    auto variables = createVariableMap(target);
    double azimuth = azimuthTree.evaluate(variables);
    
    // Handle division by zero (dx = 0)
    double dx = variables["dx"];
    if (dx == 0) {
        double dy = variables["dy"];
        if (dy > 0) azimuth = 90;
        else if (dy < 0) azimuth = 270;
        else azimuth = 0;
    }
    
    // Convert from atan range (-90 to 90) to full circle
    if (dx < 0) {
        azimuth += 180;
    } else if (variables["dy"] < 0 && dx > 0) {
        azimuth += 360;
    }
    
    // Normalize
    while (azimuth < 0) azimuth += 360;
    while (azimuth >= 360) azimuth -= 360;
    
    return azimuth;
}

FiringSolution Gun::calculateFiringSolution(const Target& target) {
    FiringSolution solution;
    
    // Calculate using expression trees
    solution.elevation = calculateElevationAngle(target);
    solution.azimuth = calculateAzimuthAngle(target);
    
    // Get traditional values
    solution.distance = target.calculateHorizontalDistance(position);
    solution.direction = target.getCompassDirectionFrom(position);
    
    // Format solution text
    solution.solutionText = formatFiringSolution(solution.elevation, 
                                                solution.azimuth, 
                                                solution.direction);
    
    // Store in history
    storeSolution(solution);
    
    return solution;
}

void Gun::storeSolution(const FiringSolution& solution) {
    // Circular buffer implementation
    solutionHistory[currentHistoryIndex] = solution;
    currentHistoryIndex = (currentHistoryIndex + 1) % historySize;
}

void Gun::getRecentSolutions(FiringSolution* output, int n) const {
    if (n > historySize) n = historySize;
    
    // Start from most recent and go backward
    int index = (currentHistoryIndex - 1 + historySize) % historySize;
    
    for (int i = 0; i < n; i++) {
        output[i] = solutionHistory[index];
        index = (index - 1 + historySize) % historySize;
        if (index < 0) index = historySize - 1;
    }
}

string Gun::formatFiringSolution(double elevation, double azimuth, const string& direction) const {
    stringstream ss;
    ss << fixed << setprecision(1);
    ss << elevation << "° " << direction;
    return ss.str();
}