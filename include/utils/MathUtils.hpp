#ifndef MATHUTILS_HPP
#define MATHUTILS_HPP

#include <cmath>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <cmath>
#include <string>
using namespace std;

//2d Vector for representing coordinated
struct Vector2D{
    double x; //x-coordinate
    double y;//y-coordinate
    Vector2D(double x=0.00, double y=0.00) : x(x), y(y){} //setting default value for both coordinates to 0.00

    double distanceTo(const Vector2D& other) const {
        double dx = x - other.x;//difference in x coordinates
        double dy = y- other.y;//difference in y coordinate
        return sqrt((dx*dx) + (dy*dy));
    }
};

//class for common math operations
class MathUtils{
    public:
    //covert angel from degrees to radian 
    static double degreesToRadian(double degrees){
        return degrees * (M_PI/180.0);//M_PI is the constant PI
    }
    //convert angel from rad to deg
    static double radiansToDegrees(double radians) {
        return radians * (180.0 / M_PI);
    }

    //calculate bearing
    static double calculateBearing(const Vector2D& origin, const Vector2D& target){
        //calculate difference in coordinates
        double dx = target.x - origin.x;//horizontal distance
        double dy = target.y - origin.y;//vertical distance

        // atan2 returns angle in radians between -π and π (-180° to 180°)
        // atan2(dy, dx) gives angle from positive x-axis
        double angleRad = atan2(dy, dx);

        //convert rad to deg
        double angleDeg = radiansToDegrees(angleRad);

         // Normalize from (-180° to 180°) range to (0° to 360°) range
        // If angle is negative, add 360° to make it positive
        if (angleDeg<0)
        {
            angleDeg+=360.0;
        }
        return angleDeg;
    }
    //Convert bearing angle to compass direction (N, NE, E, SE, S, SW, W, NW)
    static string bearingToCompassDirection(double bearing);
};
#endif