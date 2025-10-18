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