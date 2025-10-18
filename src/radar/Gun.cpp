#include "../../include/radar/Gun.hpp"
#include <cmath>

Gun::Gun(const Vector2D& pos) : position(pos) {}

FiringSolution Gun::calculateFiringSolution(const Target& target) const {
    FiringSolution solution;
    
    // Calculate horizontal distance
    solution.distance = target.calculateHorizontalDistance(position);
    
    // Calculate elevation angle (considering target height)
    solution.elevation = calculateElevationAngle(target);
    
    // Calculate azimuth (bearing)
    solution.azimuth = target.calculateBearingFrom(position);
    
    // Get compass direction
    solution.direction = target.getCompassDirectionFrom(position);
    
    return solution;
}

double Gun::calculateElevationAngle(const Target& target) const {
    double horizontalDist = target.calculateHorizontalDistance(position);
    double height = target.getHeight();
    
    if (horizontalDist == 0) {
        return 90.0; // Straight up if directly above
    }
    
    // tan(θ) = opposite/adjacent = height/horizontalDistance
    double angleRad = std::atan(height / horizontalDist);
    return MathUtils::radiansToDegrees(angleRad);
}