#ifndef TARGET_HPP
#define TARGET_HPP

#include "../utils/MathUtils.hpp"
#include <string>

enum class TargetType {
    UNKNOWN,
    FRIENDLY
};

class Target {
private:
    Vector2D position;
    double height;
    TargetType type;
    std::string id;

public:
    Target(const Vector2D& pos = Vector2D(), double h = 0.0, 
           TargetType t = TargetType::UNKNOWN, const std::string& id = "");
    
    // Getters
    Vector2D getPosition() const { return position; }
    double getHeight() const { return height; }
    TargetType getType() const { return type; }
    std::string getId() const { return id; }
    
    // Setters
    void setPosition(const Vector2D& newPos) { position = newPos; }
    void setHeight(double newHeight) { height = newHeight; }
    void setType(TargetType newType) { type = newType; }
    
    // Calculate horizontal distance from origin
    double calculateHorizontalDistance(const Vector2D& origin) const;
    
    // Calculate displacement (3D distance) from origin
    double calculateDisplacement(const Vector2D& origin) const;
    
    // Calculate bearing from origin in degrees
    double calculateBearingFrom(const Vector2D& origin) const;
    
    // Get compass direction from origin
    std::string getCompassDirectionFrom(const Vector2D& origin) const;
};

#endif