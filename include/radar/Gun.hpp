#ifndef GUN_HPP
#define GUN_HPP

#include "../utils/MathUtils.hpp"
#include "Target.hpp"
#include <string>
using namespace std;


struct FiringSolution {
    double elevation; // angle from ground in degrees

    double azimuth; // bearing angle in degrees
    string direction; // compass direction
    double distance; // Horizontal distance to target 
};

class Gun {
private:
    Vector2D position;
    
public:
    Gun(const Vector2D& pos = Vector2D(0,0));

    // Getters
    Vector2D getposition() const { return position; }

    // Calculate firing solution for a target 
    FiringSolution calculateFiringSolution(const Target& target) const;

    // Calculate required elevation angle for target(considering height)
    double calculateElevationAngle(const Target& target) const;

};

#endif