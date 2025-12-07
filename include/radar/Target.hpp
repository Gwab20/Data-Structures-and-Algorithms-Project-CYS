#ifndef TARGET_HPP
#define TARGET_HPP

#include "../utils/MathUtils.hpp"
#include <string>
#include <iostream>
using namespace std;



struct DetectionEvent {
    string targetId; //stores unique identifier for targets that are detected
    bool isEntry; //indicator to tell if target is entering range
    double timestamp;//store time of detection
};

//Queue node for detection event
struct QueueNode{
    DetectionEvent data; //Stores(in data var) the actual detection event in the queue node.
    QueueNode* next; //point to the next node
    QueueNode(const DetectionEvent& event) : data(event), next(nullptr){}
};

//circular linked list for sweep node
struct SweepNode{
    double angle; //stores angular position
    SweepNode* next;//points to the next angle/position
    SweepNode(double a): angle(a), next(nullptr){} //constructor to initilize values
};

// type of target: enemy or foe
enum class TargetType {
    UNKNOWN,
    FRIENDLY
};

// represents a moving object that radar can detect
class Target {
private:
    Vector2D position; // current location (x,y)
    double height; //altitude
    TargetType type; //friend or unknown
    std::string id; //ID

    // Phase 2 - Kinematics
    static const int HISTORY_SIZE = 10; // last 10 positions
    Vector2D positionHistory[HISTORY_SIZE]; // 
    int historyIndex; // where to store next
    Vector2D velocity; // current speed
    Vector2D acceleration; // current acceleration
    Vector2D prevVelocity;  // Changed from static to member variable
    double lastUpdateTime; // when last updated
    bool hasPreviousData;  // does it have enough data to calculate

public:
    Target(const Vector2D& pos = Vector2D(), double h = 0.0, 
           TargetType t = TargetType::UNKNOWN, const string& id = "");
    
    // Getters
    Vector2D getPosition() const { return position; }
    double getHeight() const { return height; }
    TargetType getType() const { return type; }
    std::string getId() const { return id; }
    
    // Setters
    void setPosition(const Vector2D& newPos) { position = newPos; }
    void setHeight(double newHeight) { height = newHeight; }
    void setType(TargetType newType) { type = newType; }


    bool operator==(const Target& other) const {
        return id == other.id;
    }

    
    
    // Calculate horizontal distance from origin
    double calculateHorizontalDistance(const Vector2D& origin) const;
    
    // Calculate displacement (3D distance) from origin
    double calculateDisplacement(const Vector2D& origin) const;
    
    // Calculate bearing from origin in degrees
    double calculateBearingFrom(const Vector2D& origin) const;
    
    // Get compass direction from origin
    string getCompassDirectionFrom(const Vector2D& origin) const;
    
    //phase2
    void updatePosition(const Vector2D& newPos, double currentTime);
    void calculateKinematics(double currentTime);
    Vector2D getVelocity() const { return velocity; }
    Vector2D getAcceleration() const { return acceleration; }
    double getSpeed() const { return velocity.magnitude(); }


};

#endif