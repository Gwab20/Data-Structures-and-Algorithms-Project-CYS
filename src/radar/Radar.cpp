#include "../../include/radar/Radar.hpp"

Radar::Radar(const Vector2D& pos, double range) 
    : position(pos), range(range), defenseGun(pos) {}

bool Radar::isInRange(const Target& target) const {
    return target.calculateHorizontalDistance(position) <= range;
}

void Radar::analyzeTarget(const Target& target, 
                         double& horizontalDist, 
                         double& displacement, 
                         double& bearing, 
                         string& direction) const {
    horizontalDist = target.calculateHorizontalDistance(position);
    displacement = target.calculateDisplacement(position);
    bearing = target.calculateBearingFrom(position);
    direction = target.getCompassDirectionFrom(position);
}