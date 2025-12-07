#include "../../include/ui/ConsoleUI.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <windows.h>
#include <random>

using namespace std;

// Random number generator without cstdlib
class RandomGenerator {
private:
    std::mt19937 generator;
    std::uniform_real_distribution<double> distribution;
    
public:
    RandomGenerator() : generator(std::random_device{}()), 
                       distribution(0.0, 1.0) {}
    
    double getRandomDouble() {
        return distribution(generator);
    }
    
    int getRandomInt(int min, int max) {
        std::uniform_int_distribution<int> intDist(min, max);
        return intDist(generator);
    }
};

// Setup UI, clear grids, start timers
ConsoleUI::ConsoleUI() 
    : stackTop(nullptr), stackSize(0),
      refreshFront(0), refreshRear(0), refreshCount(0),
      frameRate(0.0), frameCount(0), sweepSpeed(45.0) {
    
    // Initialize grids with empty spaces
    clearGrid(radarGrid); // current frame
    clearGrid(prevRadarGrid); // Previous frame (for animation)
    
    // Initialize refresh queue
    for (int i = 0; i < MAX_REFRESH_QUEUE; i++) {
        refreshQueue[i] = "";
    }
    
    // Initialize colors if on Windows
    initColors(); // Setup terminal colors
    
    lastFrameTime = chrono::high_resolution_clock::now();
    lastSweepTime = lastFrameTime;
}

// Cleanup frame stack
ConsoleUI::~ConsoleUI() {
    clearStack();
}

void ConsoleUI::initColors() {
    // Windows console color setup
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Define color pairs
    DWORD dwMode = 0;
    GetConsoleMode(hConsole, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, dwMode);
}

// Fill grid with spaces
void ConsoleUI::clearGrid(char grid[GRID_HEIGHT][GRID_WIDTH]) {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x] = ' ';
        }
    }
}

// Copy one grid to another (for animation)
void ConsoleUI::copyGrid(char dest[GRID_HEIGHT][GRID_WIDTH], const char src[GRID_HEIGHT][GRID_WIDTH]) {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            dest[y][x] = src[y][x];
        }
    }
}

// ========== MANUAL STACK IMPLEMENTATION ==========

// Save current radar screen to stack
void ConsoleUI::pushFrame(const char grid[GRID_HEIGHT][GRID_WIDTH], double sweepAngle) {
    // Get current time
    auto now = chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    double timestamp = chrono::duration<double>(duration).count();
    
    // Create new frame node
    FrameNode* newNode = new FrameNode(grid, sweepAngle, timestamp);
    
    // If stack is full, remove oldest frame (bottom of stack)
    if (stackSize >= MAX_FRAME_STACK) {
        // Find the second last node
        if (stackTop != nullptr && stackTop->next != nullptr) {
            FrameNode* current = stackTop;
            FrameNode* prev = nullptr;
            
            // Traverse to find second last node
            while (current->next != nullptr) {
                prev = current;
                current = current->next;
            }
            
            // Remove the last node (oldest frame)
            if (prev != nullptr) {
                delete prev->next;
                prev->next = nullptr;
                stackSize--;
            }
        } else if (stackTop != nullptr) {
            // Only one node in stack
            delete stackTop;
            stackTop = nullptr;
            stackSize = 0;
        }
    }
    
    // Push new node onto stack
    newNode->next = stackTop;
    stackTop = newNode;
    stackSize++;
}

// Remove top frame (called during undo)
bool ConsoleUI::popFrame() {
    if (stackTop == nullptr) {
        return false;
    }
    
    FrameNode* temp = stackTop;
    stackTop = stackTop->next;
    delete temp;
    stackSize--;
    
    return true;
}

// Look at top frame without removing
FrameNode* ConsoleUI::peekFrame() const {
    return stackTop;
}

// Empty the stack
void ConsoleUI::clearStack() {
    while (stackTop != nullptr) {
        FrameNode* temp = stackTop;
        stackTop = stackTop->next;
        delete temp;
    }
    stackSize = 0;
}

// User presses 'U' - go back one frame
bool ConsoleUI::undoLastFrame() {
    // Need at least 2 frames to undo (current + previous)
    if (stackSize <= 1) {
        stringstream msg;
        msg << "==============================================================\n";
        msg << "               CANNOT UNDO - Need more frames                 \n";
        msg << "==============================================================";
        enqueueRefresh(msg.str());
        return false;
    }
    
    // Pop current frame
    if (!popFrame()) {
        return false;
    }
    
    // Get previous frame (now at top)
    FrameNode* prevFrame = peekFrame();
    if (prevFrame == nullptr) {
        return false;
    }
    
    // Restore grid from previous frame
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            radarGrid[y][x] = prevFrame->grid[y][x];
        }
    }
    
    // Add undo message
    stringstream msg;
    msg << "==============================================================\n";
    msg << "               FRAME UNDO - Sweep: " 
        << setw(6) << setprecision(1) << fixed << prevFrame->sweepAngle << " deg            \n";
    msg << "==============================================================";
    enqueueRefresh(msg.str());
    
    return true;
}

// ========== ANIMATION METHODS ==========

// In ConsoleUI.cpp, update the shouldAdvanceSweep method:

bool ConsoleUI::shouldAdvanceSweep() {
    // If sweep speed is 0, never advance (true manual mode)
    if (sweepSpeed <= 0.0) {
        return false;
    }
    
    auto currentTime = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = currentTime - lastSweepTime;
    
    // Calculate if enough time has passed for sweep advancement
    // Based on sweepSpeed (degrees per second)
    double timePerDegree = 1.0 / sweepSpeed;
    
    if (elapsed.count() >= timePerDegree) {
        lastSweepTime = currentTime;
        return true;
    }
    
    return false;
}

void ConsoleUI::worldToGrid(const Vector2D& worldPos, const Radar& radar, 
                           int& gridX, int& gridY) const {
    // Convert world coordinates to grid coordinates
    // Center of grid represents radar position
    double radarRange = radar.getRange();
    Vector2D radarPos = radar.getPosition();
    
    // Normalize position relative to radar range
    double normalizedX = (worldPos.x - radarPos.x) / radarRange;
    double normalizedY = (worldPos.y - radarPos.y) / radarRange;
    
    // Convert to grid coordinates (origin at center)
    gridX = static_cast<int>((normalizedX + 1.0) * (GRID_WIDTH - 1) / 2.0);
    gridY = static_cast<int>((1.0 - normalizedY) * (GRID_HEIGHT - 1) / 2.0);
    
    // Clamp to grid bounds
    gridX = max(0, min(GRID_WIDTH - 1, gridX));
    gridY = max(0, min(GRID_HEIGHT - 1, gridY));
}

// Draw radar range circles
void ConsoleUI::drawRadarCircle(char grid[GRID_HEIGHT][GRID_WIDTH]) {
    int centerX = GRID_WIDTH / 2;
    int centerY = GRID_HEIGHT / 2;
    
    // Draw circle using parametric equations
    int radius = min(centerX, centerY) - 1;
    
    // Draw three concentric circles at 33%, 66%, 100% range
    for (int r = 1; r <= 3; r++) {
        int circleRadius = radius * r / 3;
        for (int angle = 0; angle < 360; angle += 5) {
            double rad = angle * M_PI / 180.0;
            int x = centerX + static_cast<int>(circleRadius * cos(rad));
            int y = centerY + static_cast<int>(circleRadius * sin(rad));
            
            if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
                if (r == 1) grid[y][x] = '.'; // inner circle
                else if (r == 2) grid[y][x] = ':'; //middle circle
                else grid[y][x] = '*'; // outer circle
            }
        }
    }
    
    // Draw center crosshair
    grid[centerY][centerX] = '+';
    if (centerY > 0) grid[centerY - 1][centerX] = '|';
    if (centerY < GRID_HEIGHT - 1) grid[centerY + 1][centerX] = '|';
    if (centerX > 0) grid[centerY][centerX - 1] = '-';
    if (centerX < GRID_WIDTH - 1) grid[centerY][centerX + 1] = '-';
}

// Draw N, E, S, W compass directions
void ConsoleUI::drawCompass(char grid[GRID_HEIGHT][GRID_WIDTH]) {
    // Draw compass directions at edges
    // North
    if (GRID_WIDTH / 2 >= 0 && GRID_WIDTH / 2 < GRID_WIDTH) {
        grid[0][GRID_WIDTH / 2] = 'N';
    }
    
    // South
    if (GRID_WIDTH / 2 >= 0 && GRID_WIDTH / 2 < GRID_WIDTH && 
        GRID_HEIGHT - 1 >= 0 && GRID_HEIGHT - 1 < GRID_HEIGHT) {
        grid[GRID_HEIGHT - 1][GRID_WIDTH / 2] = 'S';
    }
    
    // East
    if (GRID_WIDTH - 1 >= 0 && GRID_WIDTH - 1 < GRID_WIDTH && 
        GRID_HEIGHT / 2 >= 0 && GRID_HEIGHT / 2 < GRID_HEIGHT) {
        grid[GRID_HEIGHT / 2][GRID_WIDTH - 1] = 'E';
    }
    
    // West
    if (0 >= 0 && 0 < GRID_WIDTH && 
        GRID_HEIGHT / 2 >= 0 && GRID_HEIGHT / 2 < GRID_HEIGHT) {
        grid[GRID_HEIGHT / 2][0] = 'W';
    }
}

// Draw sweeping radar line (with animation)
void ConsoleUI::drawSweepLine(char grid[GRID_HEIGHT][GRID_WIDTH], double angle, const Radar& radar) {
    int centerX = GRID_WIDTH / 2;
    int centerY = GRID_HEIGHT / 2;
    int radius = min(centerX, centerY) - 1;
    
    // Convert angle to radians
    double rad = angle * M_PI / 180.0;
    
    // Static storage for previous sweep positions
    static int prevSweepPositions[100][2];  // Store up to 100 positions
    static int prevSweepCount = 0;
    
    // Erase previous sweep line
    for (int i = 0; i < prevSweepCount; i++) {
        int x = prevSweepPositions[i][0];
        int y = prevSweepPositions[i][1];
        
        if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
            // Restore what was there before from previous frame
            grid[y][x] = prevRadarGrid[y][x];
        }
    }
    
    // Reset for new sweep line
    prevSweepCount = 0;
    
    // Draw new sweep line using simple ray casting
    for (int r = 1; r <= radius && prevSweepCount < 100; r++) {
        int x = centerX + static_cast<int>(r * cos(rad));
        int y = centerY - static_cast<int>(r * sin(rad)); // Negative because screen Y increases downward
        
        if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
            // Store position for erasing next frame
            prevSweepPositions[prevSweepCount][0] = x;
            prevSweepPositions[prevSweepCount][1] = y;
            prevSweepCount++;
            
            // Use different characters for sweep effect
            char sweepChar;
            int pattern = r % 3;
            if (pattern == 0) sweepChar = '\\';
            else if (pattern == 1) sweepChar = '/';
            else sweepChar = '|';
            
            // Only overwrite if not a target or fixed radar element
            char current = grid[y][x];
            if (current == ' ' || current == '.' || current == ':' || current == '*' || 
                current == 'N' || current == 'S' || current == 'E' || current == 'W') {
                grid[y][x] = sweepChar;
            }
        }
    }
}

// Draw targets on radar (X = unknown, F = friendly)
void ConsoleUI::drawTargets(char grid[GRID_HEIGHT][GRID_WIDTH], const vector<Target>& targets, const Radar& radar) {
    for (const Target& target : targets) {
        if (!radar.isInRange(target)) {
            continue; // Skip targets out of range
        }
        
        int gridX, gridY;
        worldToGrid(target.getPosition(), radar, gridX, gridY);
        
        // Choose symbol based on target type
        char symbol;
        switch (target.getType()) {
            case TargetType::FRIENDLY:
                symbol = 'F'; // Friendly
                break;
            case TargetType::UNKNOWN:
            default:
                symbol = 'X'; // Unknown/Enemy
                break;
        }
        
        // Ensure we're within bounds
        if (gridX >= 0 && gridX < GRID_WIDTH && gridY >= 0 && gridY < GRID_HEIGHT) {
            grid[gridY][gridX] = symbol;
        }
    }
}

// Draw info panel (HUD) at bottom
void ConsoleUI::drawHUD(const Radar& radar, const vector<Target>& targets) {
    // Count targets by type
    int unknownCount = 0;
    int friendlyCount = 0;
    int inRangeCount = 0;
    
    for (const Target& target : targets) {
        if (target.getType() == TargetType::UNKNOWN) unknownCount++;
        else if (target.getType() == TargetType::FRIENDLY) friendlyCount++;
        
        if (radar.isInRange(target)) inRangeCount++;
    }
    
    // Prepare HUD lines
    stringstream hud;
    hud << fixed << setprecision(1);
    
    // Header
    hud << "==============================================================\n";
    hud << "                    AIR DEFENSE RADAR SYSTEM                  \n";
    hud << "==============================================================\n";
    
    // System status
    string status = (inRangeCount > 0) ? "ACTIVE TRACKING" : "SCANNING";
    int statusPadding = 49 - status.length();
    hud << " Status: " << status << string(max(0, statusPadding), ' ') << "\n";
    
    // Radar info
    string radarInfo = "Radar Position: (" + to_string(static_cast<int>(radar.getPosition().x)) + 
                       ", " + to_string(static_cast<int>(radar.getPosition().y)) + 
                       ") Range: " + to_string(static_cast<int>(radar.getRange()));
    int radarPadding = 49 - radarInfo.length();
    hud << " " << radarInfo << string(max(0, radarPadding), ' ') << "\n";
    
    // Target summary
    string targetInfo = "Targets - Total: " + to_string(targets.size()) + 
                        " In Range: " + to_string(inRangeCount) + 
                        " Unknown: " + to_string(unknownCount) + 
                        " Friendly: " + to_string(friendlyCount);
    int targetPadding = 49 - targetInfo.length();
    hud << " " << targetInfo << string(max(0, targetPadding), ' ') << "\n";
    
    // Frame rate
    string fpsInfo = "Frame Rate: " + formatDouble(frameRate, 1) + " fps";
    int fpsPadding = 49 - fpsInfo.length();
    hud << " " << fpsInfo << string(max(0, fpsPadding), ' ') << "\n";
    
    hud << "==============================================================";
    
    // Add to refresh queue
    enqueueRefresh(hud.str());
}

void ConsoleUI::enqueueRefresh(const string& message) {
    if (isRefreshQueueFull()) {
        dequeueRefresh(); // Remove oldest if full
    }
    
    refreshQueue[refreshRear] = message;
    refreshRear = (refreshRear + 1) % MAX_REFRESH_QUEUE;
    refreshCount++;
}

string ConsoleUI::dequeueRefresh() {
    if (isRefreshQueueEmpty()) {
        return "";
    }
    
    string message = refreshQueue[refreshFront];
    refreshFront = (refreshFront + 1) % MAX_REFRESH_QUEUE;
    refreshCount--;
    
    return message;
}

bool ConsoleUI::isRefreshQueueEmpty() const {
    return refreshCount == 0;
}

bool ConsoleUI::isRefreshQueueFull() const {
    return refreshCount == MAX_REFRESH_QUEUE;
}

// UPDATED VERSION - Now includes autoSweep parameter
void ConsoleUI::renderRadarDisplay(const Radar& radar, const vector<Target>& targets, bool autoSweep) {
    // Store previous frame before changes (for sweep line erasing)
    copyGrid(prevRadarGrid, radarGrid);
    
    // Clear the current grid
    clearGrid(radarGrid);
    
    // Draw radar elements
    drawRadarCircle(radarGrid);
    drawCompass(radarGrid);
    
    // Get current sweep angle
    double sweepAngle = radar.getCurrentSweepAngle();
    
    // Draw sweep line with animation
    drawSweepLine(radarGrid, sweepAngle, radar);
    
    // Draw targets
    drawTargets(radarGrid, targets, radar);
    
    // Push current frame to manual stack for undo functionality
    pushFrame(radarGrid, sweepAngle);
    
    // Print the radar grid
    clearScreen();
    
    // Draw radar display with animation effects
    cout << "==============================================================\n";
    cout << "                   LIVE RADAR DISPLAY (Phase 6)               \n";
    cout << "==============================================================\n";
    
    // Animated border effect using static counter
    static int borderAnim = 0;
    borderAnim = (borderAnim + 1) % 60;  // Slower animation
    
    // Draw the radar grid
    for (int y = 0; y < GRID_HEIGHT; y++) {
        // Animated border character
        char leftBorder = '|';
        char rightBorder = '|';
        
        // Pulsing effect on borders
        if ((borderAnim / 10) % 2 == 0) {
            if (y % 3 == 0) {
                leftBorder = '#';
                rightBorder = '#';
            }
        }
        
        cout << leftBorder;
        
        // Display grid row
        for (int x = 0; x < GRID_WIDTH; x++) {
            char displayChar = radarGrid[y][x];
            
            // Special handling for sweep line characters (make them "pulse")
            if (displayChar == '\\' || displayChar == '/' || displayChar == '|') {
                // Create a pulsing effect based on frame count
                static int pulseCounter = 0;
                pulseCounter++;
                
                // Every 3 frames, change intensity
                if ((pulseCounter / 3) % 2 == 0) {
                    // Bright sweep line
                    cout << displayChar;
                } else {
                    // Dim sweep line - use different character
                    if (displayChar == '\\') cout << '\\';
                    else if (displayChar == '/') cout << '/';
                    else cout << '|';
                }
            } else {
                cout << displayChar;
            }
        }
        
        cout << rightBorder << "\n";
    }
    
    // Bottom section with stack info
    cout << "--------------------------------------------------------------\n";
    cout << " Stack: " << setw(2) << stackSize << "/" << MAX_FRAME_STACK 
         << " frames | Sweep: " << setw(6) << setprecision(1) << fixed << sweepAngle 
         << " deg | Speed: " << setw(5) << sweepSpeed << " deg/sec" << " \n";
    
    // MODE INDICATOR - Clear visual indicator of current mode
    cout << "--------------------------------------------------------------\n";
    if (autoSweep) {
        cout << "\033[32m";  // Green for AUTO mode
        cout << " MODE: AUTO (Sweep advancing automatically)";
    } else {
        cout << "\033[33m";  // Yellow for MANUAL mode
        cout << " MODE: MANUAL (Use LEFT/RIGHT arrows to control sweep)";
    }
    cout << "\033[0m\n";  // Reset color
    
    // Control reminder
    cout << " Controls: SPACE=toggle mode | U=undo | +/-=speed | ESC=exit\n";
    cout << "==============================================================\n";
    
    // Animation status bar
    string animBar = "[";
    int barLength = 20;
    int filled = (borderAnim % barLength);
    for (int i = 0; i < barLength; i++) {
        if (i <= filled) animBar += "=";
        else animBar += ".";
    }
    animBar += "]";
    
    // Show animation progress for auto mode
    if (autoSweep) {
        cout << " Auto Sweep Progress: " << animBar << "\n";
    }
    
    // Draw HUD
    drawHUD(radar, targets);
    
    // Display refresh queue messages (firing solutions will appear here)
    while (!isRefreshQueueEmpty()) {
        string msg = dequeueRefresh();
        // Don't show "SWEEP ADVANCING" messages in manual mode
        if (!autoSweep && msg.find("SWEEP ADVANCING") != string::npos) {
            continue;
        }
        cout << msg << "\n";
    }
    
    // Update frame rate
    updateFrameRate();
    
    // Only show sweep advancement message in auto mode
    if (autoSweep && shouldAdvanceSweep()) {
        stringstream animMsg;
        animMsg << "==============================================================\n";
        animMsg << "               AUTO SWEEP ADVANCING - " 
                << setw(6) << setprecision(1) << fixed << sweepSpeed << " deg/sec            \n";
        animMsg << "==============================================================";
        cout << animMsg.str() << "\n";
    }
}

void ConsoleUI::renderTargetInfo(const Target& target, const Radar& radar) {
    stringstream info;
    
    // Calculate target data
    double horizontalDist, displacement, bearing;
    string direction;
    radar.analyzeTarget(target, horizontalDist, displacement, bearing, direction);
    
    info << fixed << setprecision(1);
    info << "==============================================================\n";
    info << "                     TARGET INFORMATION                       \n";
    info << "--------------------------------------------------------------\n";
    
     // ID and type line
    string targetIdLine = "ID: " + target.getId() + 
                         " Type: " + (target.getType() == TargetType::UNKNOWN ? "UNKNOWN" : "FRIENDLY");
    int idPadding = 49 - targetIdLine.length();  // Pad to 49 chars for alignment
    info << " " << targetIdLine << string(max(0, idPadding), ' ') << "\n";
    
       // Position and height line
    string posLine = "Position: (" + to_string(static_cast<int>(target.getPosition().x)) + 
                     ", " + to_string(static_cast<int>(target.getPosition().y)) + 
                     ") Height: " + to_string(static_cast<int>(target.getHeight()));
    int posPadding = 49 - posLine.length();
    info << " " << posLine << string(max(0, posPadding), ' ') << "\n";
    
     // Distance info
    string distLine = "Horizontal Distance: " + formatDouble(horizontalDist, 1) + 
                     " Displacement: " + formatDouble(displacement, 1);
    int distPadding = 49 - distLine.length();
    info << " " << distLine << string(max(0, distPadding), ' ') << "\n";
    
      // Direction info
    string bearingLine = "Bearing: " + formatDouble(bearing, 1) + 
                        " deg Direction: " + direction;
    int bearingPadding = 49 - bearingLine.length();
    info << " " << bearingLine << string(max(0, bearingPadding), ' ') << "\n";
    
       // Speed info
    string speedLine = "Speed: " + formatDouble(target.getSpeed(), 1) + 
                      " Velocity: (" + formatDouble(target.getVelocity().x, 1) + 
                      ", " + formatDouble(target.getVelocity().y, 1) + ")";
    int speedPadding = 49 - speedLine.length();
    info << " " << speedLine << string(max(0, speedPadding), ' ') << "\n";
    
    info << "==============================================================";
    
    enqueueRefresh(info.str()); // Add to message queue for display
}

void ConsoleUI::renderFiringSolution(const Gun& gun, const Target& target) {
    // Now this will work because calculateFiringSolution is const
    FiringSolution solution = gun.calculateFiringSolution(target);
    
    stringstream ss;
    ss << fixed << setprecision(1);
    ss << "==============================================================\n";
    ss << "                     FIRING SOLUTION                          \n";
    ss << "--------------------------------------------------------------\n";
    
    string solutionLine = solution.solutionText;
    int solPadding = 49 - solutionLine.length();
    ss << " " << solutionLine << string(max(0, solPadding), ' ') << "\n";
    
    string detailLine = "Elevation: " + formatDouble(solution.elevation, 1) + 
                       " deg Azimuth: " + formatDouble(solution.azimuth, 1) + 
                       " deg Distance: " + formatDouble(solution.distance, 1);
    int detailPadding = 49 - detailLine.length();
    ss << " " << detailLine << string(max(0, detailPadding), ' ') << "\n";
    
    ss << "==============================================================";
    
    // Add special color for firing solutions
    string coloredMsg = "\033[31m" + ss.str() + "\033[0m";  // Red color for firing solutions
    enqueueRefresh(coloredMsg);
}

void ConsoleUI::renderSystemStatus(const Radar& radar, int totalTargets, 
                                  int detectedTargets) {
    stringstream status;
    
    status << "==============================================================\n";
    status << "                     SYSTEM STATUS (Phase 6)                 \n";
    status << "--------------------------------------------------------------\n";
    
        // Line 1: Radar sweep info
    string sweepLine = "Sweep Angle: " + formatDouble(radar.getCurrentSweepAngle(), 1) + 
                      " deg Speed: " + formatDouble(sweepSpeed, 1) + " deg/sec";
    int sweepPadding = 49 - sweepLine.length();
    status << " " << sweepLine << string(max(0, sweepPadding), ' ') << "\n";
    
     // Line 2: Frame and performance stats
    string frameLine = "Frame Stack: " + to_string(stackSize) + 
                      "/" + to_string(MAX_FRAME_STACK) + 
                      " FPS: " + formatDouble(frameRate, 1);
    int framePadding = 49 - frameLine.length();
    status << " " << frameLine << string(max(0, framePadding), ' ') << "\n";
    
     // Line 3: Target tracking stats
    string targetLine = "Targets Tracked: " + to_string(detectedTargets) + 
                       "/" + to_string(totalTargets);
    int targetPadding = 49 - targetLine.length();
    status << " " << targetLine << string(max(0, targetPadding), ' ') << "\n";
    
       // Line 4: Undo stack info with control hint
    string stackInfo = "Stack Depth: " + to_string(stackSize) + 
                      " (Press 'U' to undo)";
    int stackPadding = 49 - stackInfo.length();
    status << " " << stackInfo << string(max(0, stackPadding), ' ') << "\n";
    
    status << "==============================================================";
    
    enqueueRefresh(status.str()); // Send to display queue
}

void ConsoleUI::updateFrameRate() {
    auto currentTime = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = currentTime - lastFrameTime;
    
    frameCount++;
    
    // Update frame rate every second
    if (elapsed.count() >= 1.0) {
        frameRate = frameCount / elapsed.count(); // frames per second
        frameCount = 0; // reset counter
        lastFrameTime = currentTime; // reset timer
    }
}

void ConsoleUI::clearScreen() {
    // Use ANSI escape codes for cross-platform screen clearing
    cout << "\033[2J\033[1;1H";
}

void ConsoleUI::setCursorPosition(int x, int y) {
    // Use ANSI escape codes for cursor positioning
    cout << "\033[" << y + 1 << ";" << x + 1 << "H";
}

string ConsoleUI::formatDouble(double value, int precision) {
    stringstream ss;
    ss << fixed << setprecision(precision) << value; // format: 45.0
    return ss.str();
}