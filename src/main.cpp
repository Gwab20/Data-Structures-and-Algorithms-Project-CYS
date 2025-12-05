#include <iostream>
#include <iomanip>
#include <windows.h>
#include "../include/radar/Target.hpp"
#include "../include/radar/Gun.hpp"

using namespace std;

// Function to set up console for UTF-8
void setupConsoleUTF8() {
    // For Windows: Set console output to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    
    // Also try setting input encoding to UTF-8 (optional)
    SetConsoleCP(CP_UTF8);
    
    // Enable virtual terminal processing for better Unicode support
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}

void testExpressionTreeManual() {
    cout << "=== TESTING EXPRESSION TREE (MANUAL MEMORY) ===" << endl;
    
    // Create a simple expression: (x + y) * 2
    ExpressionNode* xNode = new VariableNode("x");
    ExpressionNode* yNode = new VariableNode("y");
    ExpressionNode* addNode = new OperatorNode('+', xNode, yNode);
    ExpressionNode* twoNode = new ConstantNode(2.0);
    ExpressionNode* multiplyNode = new OperatorNode('*', addNode, twoNode);
    
    ExpressionTree testTree(multiplyNode);
    
    map<string, double> vars;
    vars["x"] = 10.0;
    vars["y"] = 5.0;
    
    double result = testTree.evaluate(vars);
    cout << "(x + y) * 2 with x=10, y=5 = " << result << endl;
    cout << "Expected: " << (10.0 + 5.0) * 2.0 << endl;
    
    // Test cloning
    ExpressionTree clonedTree = testTree;
    vars["x"] = 3.0;
    vars["y"] = 7.0;
    double clonedResult = clonedTree.evaluate(vars);
    cout << "Cloned tree with x=3, y=7 = " << clonedResult << endl;
}

void testGunWithDynamicArrays() {
    cout << "\n=== TESTING GUN WITH DYNAMIC ARRAYS ===" << endl;
    
    Gun gun(Vector2D(0, 0));
    
    // Create array of test targets
    const int NUM_TARGETS = 5;
    Target* targets = new Target[NUM_TARGETS];
    
    targets[0] = Target(Vector2D(100, 100), 500, TargetType::UNKNOWN, "T1");
    targets[1] = Target(Vector2D(200, 0), 800, TargetType::UNKNOWN, "T2");
    targets[2] = Target(Vector2D(0, 300), 600, TargetType::UNKNOWN, "T3");
    targets[3] = Target(Vector2D(-150, 150), 400, TargetType::UNKNOWN, "T4");
    targets[4] = Target(Vector2D(50, -200), 700, TargetType::UNKNOWN, "T5");
    
    cout << fixed << setprecision(1);
    
    // Calculate solutions for all targets
    for (int i = 0; i < NUM_TARGETS; i++) {
        FiringSolution solution = gun.calculateFiringSolution(targets[i]);
        
        cout << "Target " << (i+1) << " (" << targets[i].getId() << "): ";
        cout << solution.solutionText << endl;
        cout << "  Details - Elev: " << solution.elevation 
             << "°, Azim: " << solution.azimuth 
             << "°, Dist: " << solution.distance << endl;
    }
    
    // Test history retrieval
    cout << "\n=== TESTING SOLUTION HISTORY ===" << endl;
    FiringSolution* recentSolutions = new FiringSolution[3];
    gun.getRecentSolutions(recentSolutions, 3);
    
    cout << "Last 3 solutions:" << endl;
    for (int i = 0; i < 3; i++) {
        cout << "  " << recentSolutions[i].solutionText << endl;
    }
    
    // Clean up
    delete[] targets;
    delete[] recentSolutions;
}

void testComplexExpression() {
    cout << "\n=== TESTING COMPLEX EXPRESSION TREE ===" << endl;
    
    // Create: sqrt(x² + y²) * sin(angle)
    ExpressionNode* xNode = new VariableNode("x");
    ExpressionNode* xSquared = new OperatorNode('^', xNode->clone(), new ConstantNode(2));
    
    ExpressionNode* yNode = new VariableNode("y");
    ExpressionNode* ySquared = new OperatorNode('^', yNode->clone(), new ConstantNode(2));
    
    ExpressionNode* sumNode = new OperatorNode('+', xSquared, ySquared);
    ExpressionNode* sqrtNode = new FunctionNode("sqrt", sumNode);
    
    ExpressionNode* angleNode = new VariableNode("angle");
    ExpressionNode* sinNode = new FunctionNode("sin", angleNode);
    
    ExpressionNode* finalNode = new OperatorNode('*', sqrtNode, sinNode);
    
    ExpressionTree complexTree(finalNode);
    
    map<string, double> vars;
    vars["x"] = 3.0;
    vars["y"] = 4.0;
    vars["angle"] = M_PI / 6; // 30 degrees in radians
    
    double result = complexTree.evaluate(vars);
    cout << "sqrt(x² + y²) * sin(angle) with x=3, y=4, angle=30° = " << result << endl;
    cout << "Expected: sqrt(9+16) * sin(30°) = 5 * 0.5 = 2.5" << endl;
}

int main() {
    
    // Setup console encoding FIRST, before any output
    setupConsoleUTF8();

    testExpressionTreeManual();
    testGunWithDynamicArrays();
    testComplexExpression();
    
    return 0;
}