#include "../../include/radar/Target.hpp"
#include <cmath>

// Create a target with position, height , type , ID
Target::Target(const Vector2D& pos, double h, TargetType t, const std::string& id)
    : position(pos), height(h), type(t), id(id), 
      historyIndex(0), velocity(Vector2D(0,0)), acceleration(Vector2D(0,0)),
      prevVelocity(Vector2D(0,0)), lastUpdateTime(0.0), hasPreviousData(false) 
{
    // Initialize position history with current position
    for (int i = 0; i < HISTORY_SIZE; i++) {
        positionHistory[i] = pos;
    }
}

// Calculate flat distance to origin (ignoring height)
double Target::calculateHorizontalDistance(const Vector2D& origin) const {
    return position.distanceTo(origin);
}

// Calculate 3D distance to origin (includes height)
double Target::calculateDisplacement(const Vector2D& origin) const {
    double horizontalDist = calculateHorizontalDistance(origin);
    return std::sqrt(horizontalDist * horizontalDist + height * height);
}

// Get angle from origin to target (0-360 degrees)
double Target::calculateBearingFrom(const Vector2D& origin) const {
    return MathUtils::calculateBearing(origin, position);
}

// Get compass direction like "NE", "SW"
std::string Target::getCompassDirectionFrom(const Vector2D& origin) const {
    double bearing = calculateBearingFrom(origin);
    return MathUtils::bearingToCompassDirection(bearing);
}

// Move target to new position and track movement
void Target::updatePosition(const Vector2D& newPos, double currentTime){
    // Circular buffer update logic
    historyIndex = (historyIndex + 1) % HISTORY_SIZE;
    positionHistory[historyIndex] = newPos;
    position = newPos;  // Update current position
      // Update speed and acceleration based on movement
    calculateKinematics(currentTime);
}

// Calculate speed (velocity) and acceleration from position history
void Target::calculateKinematics(double currentTime) {
    double deltaTime = currentTime - lastUpdateTime;
    
    // Prevent division by zero and handle initialization
    if (deltaTime <= 0) {
        lastUpdateTime = currentTime;
        return;
    }
    
    // Need at least 2 positions to calculate velocity
    if (historyIndex >= 1) {
        int prevIndex = (historyIndex - 1 + HISTORY_SIZE) % HISTORY_SIZE;
        
        // Finite difference velocity calculation
        velocity.x = (positionHistory[historyIndex].x - positionHistory[prevIndex].x) / deltaTime;
        velocity.y = (positionHistory[historyIndex].y - positionHistory[prevIndex].y) / deltaTime;
        
        // Calculate acceleration if we have previous velocity data
        if (hasPreviousData && deltaTime > 0) {
            acceleration.x = (velocity.x - prevVelocity.x) / deltaTime;
            acceleration.y = (velocity.y - prevVelocity.y) / deltaTime;
        }
        
        // Store current velocity for next acceleration calculation
        prevVelocity = velocity;
        hasPreviousData = true;
    }
    
    lastUpdateTime = currentTime;
}