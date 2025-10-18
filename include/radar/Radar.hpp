#ifndef RADAR_HPP
#define RADAR_HPP

#include "../utils/MathUtils.hpp"
#include "Target.hpp"
#include "Gun.hpp"
#include <vector>
using namespace std;

//Radar class that detects targets, calculates their position data, and controls defense systems
class Radar {
    private:
    Vector2D position;//Radar's own position in 2D space (x, y coordinates)
    double range;//Maximum detection radius (how far the radar can "see")
    Gun defencegun; //The defensive weapon system controlled by this radar
    
    //the initial position of radar(0,0) and its range(1000)
    public:
    Radar(const Vector2D& pos = Vector2D(0, 0), double range = 1000.0);

        Vector2D getPosition() const { return position; }
        Gun getDefenseGun() const { return defenseGun; }
        bool isInRange(const Target& target) const; //Uses Euclidean distance calculation between radar position and target position. If distance <= radar range, the target is detectable.
        
        /*This is the main analysis function that calculates all essential targeting data:
          Horizontal distance: Straight-line range to target
          Displacement: Magnitude of position difference vector  
          Bearing: Precise angle from radar to target
          Direction: Human-readable compass direction
     
         These outputs are used for targeting, tracking, and engagement decisions.*/
         void analyzeTarget(const Target& target, 
                      double& horizontalDist, 
                      double& displacement, 
                      double& bearing, 
                      string& direction) const;


};
#endif