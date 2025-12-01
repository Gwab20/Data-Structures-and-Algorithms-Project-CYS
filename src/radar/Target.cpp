#include "../../include/radar/Target.hpp"
#include <cmath>

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

double Target::calculateHorizontalDistance(const Vector2D& origin) const {
    return position.distanceTo(origin);
}

double Target::calculateDisplacement(const Vector2D& origin) const {
    double horizontalDist = calculateHorizontalDistance(origin);
    return std::sqrt(horizontalDist * horizontalDist + height * height);
}

double Target::calculateBearingFrom(const Vector2D& origin) const {
    return MathUtils::calculateBearing(origin, position);
}

std::string Target::getCompassDirectionFrom(const Vector2D& origin) const {
    double bearing = calculateBearingFrom(origin);
    return MathUtils::bearingToCompassDirection(bearing);
}

void Target::updatePosition(const Vector2D& newPos, double currentTime){
    // Circular buffer update logic
    historyIndex = (historyIndex + 1) % HISTORY_SIZE;
    positionHistory[historyIndex] = newPos;
    position = newPos;
    calculateKinematics(currentTime);
}

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