#include "../../include/utils/MathUtils.hpp"

string MathUtils::bearingToCompassDirection(double bearing) {
    // Normalize bearing to 0-360
    bearing = fmod(bearing, 360.0);
    if (bearing < 0) bearing += 360.0;
    
    // Define compass sectors
    const char* directions[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    
    // Each sector is 45 degrees wide
    int index = static_cast<int>((bearing + 22.5) / 45.0) % 8;
    
    return directions[index];
}


double MathUtils::calculateSpeed(const Vector2D& velocity) {
    return velocity.magnitude();
}
// === PHASE 4: Expression Tree Factory Methods ===
ExpressionTree ExpressionTree::createElevationExpression() {
    // elevation = atan(height / distance) * (180 / π)
    ExpressionNode* heightNode = new VariableNode("height");
    ExpressionNode* distanceNode = new VariableNode("distance");
    ExpressionNode* divisionNode = new OperatorNode('/', heightNode, distanceNode);
    ExpressionNode* atanNode = new FunctionNode("atan", divisionNode);
    ExpressionNode* constantNode = new ConstantNode(180.0 / M_PI);
    ExpressionNode* root = new OperatorNode('*', atanNode, constantNode);
    
    return ExpressionTree(root);
}

ExpressionTree ExpressionTree::createAzimuthExpression() {
    // azimuth = atan(dy/dx) * (180 / π), normalized later
    ExpressionNode* dyNode = new VariableNode("dy");
    ExpressionNode* dxNode = new VariableNode("dx");
    ExpressionNode* divisionNode = new OperatorNode('/', dyNode, dxNode);
    ExpressionNode* atanNode = new FunctionNode("atan", divisionNode);
    ExpressionNode* constantNode = new ConstantNode(180.0 / M_PI);
    ExpressionNode* root = new OperatorNode('*', atanNode, constantNode);
    
    return ExpressionTree(root);
}