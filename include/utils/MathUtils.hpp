#ifndef MATHUTILS_HPP
#define MATHUTILS_HPP

#include <cmath>
#include <string>
#include <vector>
#include <map>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <cmath>
#include <string>
using namespace std;

//2d Vector for representing coordinates
struct Vector2D{
    double x; //x-coordinate
    double y;//y-coordinate
    Vector2D(double x=0.00, double y=0.00) : x(x), y(y){} //setting default value for both coordinates to 0.00

    double distanceTo(const Vector2D& other) const {
        double dx = x - other.x;//difference in x coordinates
        double dy = y - other.y;//difference in y coordinate
        return sqrt((dx*dx) + (dy*dy));
    }

    //phase 2
    Vector2D operator-(const Vector2D& other) const {
        return Vector2D(x - other.x, y - other.y);
    }

    Vector2D operator/(double scalar) const {
        if (scalar != 0) return Vector2D(x / scalar, y / scalar);
        return Vector2D(0, 0);
    }

    double magnitude() const {
        return sqrt(x*x + y*y);
    }
};

//class for common math operations
class MathUtils{
    public:
    //convert angle from degrees to radian 
    static double degreesToRadian(double degrees){
        return degrees * (M_PI/180.0);//M_PI is the constant PI
    }
    //convert angle from rad to deg
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

    //phase 2 - ADD THIS FUNCTION DECLARATION
    static double calculateSpeed(const Vector2D& velocity);
    
};

//  PHASE 4: Expression Tree Nodes (Manual Memory Management) 
enum class NodeType {
    CONSTANT,
    VARIABLE,
    OPERATOR,
    FUNCTION
};

class ExpressionNode {
public:
    virtual ~ExpressionNode() {}
    virtual double evaluate(const map<string, double>& variables) const = 0;
    virtual NodeType getType() const = 0;
    virtual ExpressionNode* clone() const = 0; // For copying nodes
};

class ConstantNode : public ExpressionNode {
private:
    double value;
public:
    ConstantNode(double val) : value(val) {}
    double evaluate(const std::map<std::string, double>& variables) const override {
        return value;
    }
    NodeType getType() const override { return NodeType::CONSTANT; }
    ExpressionNode* clone() const override {
        return new ConstantNode(value);
    }
};

class VariableNode : public ExpressionNode {
private:
    std::string name;
public:
    VariableNode(const std::string& varName) : name(varName) {}
    double evaluate(const std::map<std::string, double>& variables) const override {
        auto it = variables.find(name);
        if (it != variables.end()) return it->second;
        return 0.0;
    }
    NodeType getType() const override { return NodeType::VARIABLE; }
    ExpressionNode* clone() const override {
        return new VariableNode(name);
    }
};

class OperatorNode : public ExpressionNode {
private:
    char op;
    ExpressionNode* left;
    ExpressionNode* right;
    
public:
    OperatorNode(char operation, ExpressionNode* l, ExpressionNode* r)
        : op(operation), left(l), right(r) {}
    
    ~OperatorNode() {
        delete left;
        delete right;
    }
    
    double evaluate(const std::map<std::string, double>& variables) const override {
        double leftVal = left->evaluate(variables);
        double rightVal = right->evaluate(variables);
        
        switch(op) {
            case '+': return leftVal + rightVal;
            case '-': return leftVal - rightVal;
            case '*': return leftVal * rightVal;
            case '/': return rightVal != 0 ? leftVal / rightVal : 0.0;
            case '^': return pow(leftVal, rightVal);
            default: return 0.0;
        }
    }
    
    NodeType getType() const override { return NodeType::OPERATOR; }
    
    ExpressionNode* clone() const override {
        return new OperatorNode(op, left->clone(), right->clone());
    }
};

class FunctionNode : public ExpressionNode {
private:
    std::string funcName;
    ExpressionNode* argument;
    
public:
    FunctionNode(const std::string& name, ExpressionNode* arg)
        : funcName(name), argument(arg) {}
    
    ~FunctionNode() {
        delete argument;
    }
    
    double evaluate(const std::map<std::string, double>& variables) const override {
        double argVal = argument->evaluate(variables);
        
        if (funcName == "sin") return sin(argVal);
        if (funcName == "cos") return cos(argVal);
        if (funcName == "tan") return tan(argVal);
        if (funcName == "atan") return atan(argVal);
        if (funcName == "sqrt") return sqrt(argVal);
        if (funcName == "log") return log(argVal);
        
        return 0.0;
    }
    
    NodeType getType() const override { return NodeType::FUNCTION; }
    
    ExpressionNode* clone() const override {
        return new FunctionNode(funcName, argument->clone());
    }
};

// Expression Tree class
class ExpressionTree {
private:
    ExpressionNode* root;
    
    // Helper for deep copy
    ExpressionNode* copyTree(ExpressionNode* node) {
        if (node == nullptr) return nullptr;
        return node->clone();
    }
    
    // Helper for deleting tree
    void deleteTree(ExpressionNode* node) {
        if (node) {
            delete node;
        }
    }
    
public:
    // Constructor
    ExpressionTree(ExpressionNode* rootNode = nullptr) : root(rootNode) {}
    
    // Copy constructor
    ExpressionTree(const ExpressionTree& other) {
        root = copyTree(other.root);
    }
    
    // Assignment operator
    ExpressionTree& operator=(const ExpressionTree& other) {
        if (this != &other) {
            deleteTree(root);
            root = copyTree(other.root);
        }
        return *this;
    }
    
    // Destructor
    ~ExpressionTree() {
        deleteTree(root);
    }
    
    double evaluate(const std::map<std::string, double>& variables) const {
        if (root) return root->evaluate(variables);
        return 0.0;
    }
    
    // Static factory methods to create expression trees
    static ExpressionTree createElevationExpression();
    static ExpressionTree createAzimuthExpression();
};

#endif