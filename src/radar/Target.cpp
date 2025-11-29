#include "../../include/radar/Target.hpp"

Target::Target(const Vector2D& pos, double h, TargetType t, const std::string& id)
    : position(pos), height(h), type(t), id(id) {}

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
    if (deltaTime > 0 && deltaTime < 1.0) { // Prevent division by zero and large jumps
        int prevIndex = (historyIndex - 1 + HISTORY_SIZE) % HISTORY_SIZE;
        
        // Finite difference velocity calculation
        velocity.x = (positionHistory[historyIndex].x - positionHistory[prevIndex].x) / deltaTime;
        velocity.y = (positionHistory[historyIndex].y - positionHistory[prevIndex].y) / deltaTime;
        
        // Store acceleration calculation for Member 2 to complete
        
static Vector2D prevVelocity; // Static to maintain between calls
if (deltaTime > 0) {
    acceleration.x = (velocity.x - prevVelocity.x) / deltaTime;
    acceleration.y = (velocity.y - prevVelocity.y) / deltaTime;
    prevVelocity = velocity; // Update for next call
}
    }

    lastUpdateTime = currentTime;
}