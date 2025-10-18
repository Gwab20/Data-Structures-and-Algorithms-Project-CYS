#include "../../include/radar/Target.hpp"
using namespace std;

Target::Target(const Vector2D& pos, double h, TargetType t, const string& id)
    : position(pos),height(h), type(t), id(id) {}

double Target::calculateHorizontalDistance(const Vector2D& origin) const {
    return position.distanceTo(origin);
}

double Target::calculateDisplacement (const Vector2D& origin ) const {
    double horizontalDist = calculateHorizontalDistance(origin);
    return sqrt(horizontalDist * horizontalDist + height * height);
}

double Target::calculateBearingForm(const Vector2D& origin) const {
    return MathUtils::calculateBearing(origin, position);
}

string Target::getCompassDirectionFrom(const Vector2D& origin) const {
    double bearing = calculateBearingFrom(origin);
    return MathUtils::bearingToCompassDirection(bearing);
}