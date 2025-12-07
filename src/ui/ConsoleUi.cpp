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

ConsoleUI::ConsoleUI() 
    : stackTop(nullptr), stackSize(0),
      refreshFront(0), refreshRear(0), refreshCount(0),
      frameRate(0.0), frameCount(0), sweepSpeed(45.0) {
    
    // Initialize grids with empty spaces
    clearGrid(radarGrid);
    clearGrid(prevRadarGrid);
    
    // Initialize refresh queue
    for (int i = 0; i < MAX_REFRESH_QUEUE; i++) {
        refreshQueue[i] = "";
    }
    
    // Initialize colors if on Windows
    initColors();
    
    lastFrameTime = chrono::high_resolution_clock::now();
    lastSweepTime = lastFrameTime;
}

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

void ConsoleUI::clearGrid(char grid[GRID_HEIGHT][GRID_WIDTH]) {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x] = ' ';
        }
    }
}

void ConsoleUI::copyGrid(char dest[GRID_HEIGHT][GRID_WIDTH], const char src[GRID_HEIGHT][GRID_WIDTH]) {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            dest[y][x] = src[y][x];
        }
    }
}

// ========== MANUAL STACK IMPLEMENTATION ==========

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

FrameNode* ConsoleUI::peekFrame() const {
    return stackTop;
}

void ConsoleUI::clearStack() {
    while (stackTop != nullptr) {
        FrameNode* temp = stackTop;
        stackTop = stackTop->next;
        delete temp;
    }
    stackSize = 0;
}

bool ConsoleUI::undoLastFrame() {
    // Need at least 2 frames to undo (current + previous)
    if (stackSize <= 1) {
        stringstream msg;
        msg << "╔══════════════════════════════════════════════════════════════╗\n";
        msg << "║               CANNOT UNDO - Need more frames                 ║\n";
        msg << "╚══════════════════════════════════════════════════════════════╝";
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
    msg << "╔══════════════════════════════════════════════════════════════╗\n";
    msg << "║               FRAME UNDO - Sweep: " 
        << setw(6) << setprecision(1) << fixed << prevFrame->sweepAngle << "°              ║\n";
    msg << "╚══════════════════════════════════════════════════════════════╝";
    enqueueRefresh(msg.str());
    
    return true;
}

// ========== ANIMATION METHODS ==========

bool ConsoleUI::shouldAdvanceSweep() {
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

void ConsoleUI::drawRadarCircle(char grid[GRID_HEIGHT][GRID_WIDTH]) {
    int centerX = GRID_WIDTH / 2;
    int centerY = GRID_HEIGHT / 2;
    
    // Draw circle using parametric equations
    int radius = min(centerX, centerY) - 1;
    
    // Draw three concentric circles
    for (int r = 1; r <= 3; r++) {
        int circleRadius = radius * r / 3;
        for (int angle = 0; angle < 360; angle += 5) {
            double rad = angle * M_PI / 180.0;
            int x = centerX + static_cast<int>(circleRadius * cos(rad));
            int y = centerY + static_cast<int>(circleRadius * sin(rad));
            
            if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
                if (r == 1) grid[y][x] = '.';
                else if (r == 2) grid[y][x] = ':';
                else grid[y][x] = '*';
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
    hud << "╔══════════════════════════════════════════════════════════════╗\n";
    hud << "║                    AIR DEFENSE RADAR SYSTEM                  ║\n";
    hud << "╠══════════════════════════════════════════════════════════════╣\n";
    
    // System status
    string status = (inRangeCount > 0) ? "ACTIVE TRACKING" : "SCANNING";
    int statusPadding = 49 - status.length();
    hud << "║ Status: " << status << string(max(0, statusPadding), ' ') << "║\n";
    
    // Radar info
    string radarInfo = "Radar Position: (" + to_string(static_cast<int>(radar.getPosition().x)) + 
                       ", " + to_string(static_cast<int>(radar.getPosition().y)) + 
                       ") Range: " + to_string(static_cast<int>(radar.getRange()));
    int radarPadding = 49 - radarInfo.length();
    hud << "║ " << radarInfo << string(max(0, radarPadding), ' ') << "║\n";
    
    // Target summary
    string targetInfo = "Targets - Total: " + to_string(targets.size()) + 
                        " In Range: " + to_string(inRangeCount) + 
                        " Unknown: " + to_string(unknownCount) + 
                        " Friendly: " + to_string(friendlyCount);
    int targetPadding = 49 - targetInfo.length();
    hud << "║ " << targetInfo << string(max(0, targetPadding), ' ') << "║\n";
    
    // Frame rate
    string fpsInfo = "Frame Rate: " + formatDouble(frameRate, 1) + " fps";
    int fpsPadding = 49 - fpsInfo.length();
    hud << "║ " << fpsInfo << string(max(0, fpsPadding), ' ') << "║\n";
    
    hud << "╚══════════════════════════════════════════════════════════════╝";
    
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

void ConsoleUI::renderRadarDisplay(const Radar& radar, const vector<Target>& targets) {
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
    cout << "╔══════════════════════════════════════════════════════════════╗\n";
    cout << "║                   LIVE RADAR DISPLAY (Phase 6)               ║\n";
    cout << "╠══════════════════════════════════════════════════════════════╣\n";
    
    // Animated border effect using static counter
    static int borderAnim = 0;
    borderAnim = (borderAnim + 1) % 60;  // Slower animation
    
    // Draw the radar grid
    for (int y = 0; y < GRID_HEIGHT; y++) {
        // Animated border character
        char leftBorder = '║';
        char rightBorder = '║';
        
        // Pulsing effect on borders
        if ((borderAnim / 10) % 2 == 0) {
            if (y % 3 == 0) {
                leftBorder = '▓';
                rightBorder = '▓';
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
                    // Dim sweep line
                    cout << displayChar;  // Could use different character for dim effect
                }
            } else {
                cout << displayChar;
            }
        }
        
        cout << rightBorder << "\n";
    }
    
    // Bottom section with stack info
    cout << "╠══════════════════════════════════════════════════════════════╣\n";
    cout << "║ Stack: " << setw(2) << stackSize << "/" << MAX_FRAME_STACK 
         << " frames | Sweep: " << setw(6) << setprecision(1) << fixed << sweepAngle 
         << "° | Speed: " << setw(5) << sweepSpeed << "°/sec" << " ║\n";
    
    // Animation status bar
    string animBar = "[";
    int barLength = 20;
    int filled = (borderAnim % barLength);
    for (int i = 0; i < barLength; i++) {
        if (i <= filled) animBar += "█";
        else animBar += "░";
    }
    animBar += "]";
    
    cout << "║ Animation: " << animBar << " " << string(27, ' ') << "║\n";
    cout << "╚══════════════════════════════════════════════════════════════╝\n";
    cout << "\n";
    
    // Draw HUD
    drawHUD(radar, targets);
    
    // Display refresh queue messages
    while (!isRefreshQueueEmpty()) {
        cout << dequeueRefresh() << "\n";
    }
    
    // Update frame rate
    updateFrameRate();
    
    // Display sweep advancement status
    if (shouldAdvanceSweep()) {
        stringstream animMsg;
        animMsg << "╔══════════════════════════════════════════════════════════════╗\n";
        animMsg << "║               SWEEP ADVANCING - " 
                << setw(6) << setprecision(1) << fixed << sweepSpeed << "°/sec              ║\n";
        animMsg << "╚══════════════════════════════════════════════════════════════╝";
        enqueueRefresh(animMsg.str());
    }
}

void ConsoleUI::renderTargetInfo(const Target& target, const Radar& radar) {
    stringstream info;
    
    // Calculate target data
    double horizontalDist, displacement, bearing;
    string direction;
    radar.analyzeTarget(target, horizontalDist, displacement, bearing, direction);
    
    info << fixed << setprecision(1);
    info << "╔══════════════════════════════════════════════════════════════╗\n";
    info << "║                     TARGET INFORMATION                       ║\n";
    info << "╠══════════════════════════════════════════════════════════════╣\n";
    
    string targetIdLine = "ID: " + target.getId() + 
                         " Type: " + (target.getType() == TargetType::UNKNOWN ? "UNKNOWN" : "FRIENDLY");
    int idPadding = 49 - targetIdLine.length();
    info << "║ " << targetIdLine << string(max(0, idPadding), ' ') << "║\n";
    
    string posLine = "Position: (" + to_string(static_cast<int>(target.getPosition().x)) + 
                     ", " + to_string(static_cast<int>(target.getPosition().y)) + 
                     ") Height: " + to_string(static_cast<int>(target.getHeight()));
    int posPadding = 49 - posLine.length();
    info << "║ " << posLine << string(max(0, posPadding), ' ') << "║\n";
    
    string distLine = "Horizontal Distance: " + formatDouble(horizontalDist, 1) + 
                     " Displacement: " + formatDouble(displacement, 1);
    int distPadding = 49 - distLine.length();
    info << "║ " << distLine << string(max(0, distPadding), ' ') << "║\n";
    
    string bearingLine = "Bearing: " + formatDouble(bearing, 1) + 
                        "° Direction: " + direction;
    int bearingPadding = 49 - bearingLine.length();
    info << "║ " << bearingLine << string(max(0, bearingPadding), ' ') << "║\n";
    
    string speedLine = "Speed: " + formatDouble(target.getSpeed(), 1) + 
                      " Velocity: (" + formatDouble(target.getVelocity().x, 1) + 
                      ", " + formatDouble(target.getVelocity().y, 1) + ")";
    int speedPadding = 49 - speedLine.length();
    info << "║ " << speedLine << string(max(0, speedPadding), ' ') << "║\n";
    
    info << "╚══════════════════════════════════════════════════════════════╝";
    
    enqueueRefresh(info.str());
}

void ConsoleUI::renderFiringSolution(const Gun& gun, const Target& target) {
    // Now this will work because calculateFiringSolution is const
    FiringSolution solution = gun.calculateFiringSolution(target);
    
    stringstream ss;
    ss << fixed << setprecision(1);
    ss << "╔══════════════════════════════════════════════════════════════╗\n";
    ss << "║                     FIRING SOLUTION                          ║\n";
    ss << "╠══════════════════════════════════════════════════════════════╣\n";
    
    string solutionLine = solution.solutionText;
    int solPadding = 49 - solutionLine.length();
    ss << "║ " << solutionLine << string(max(0, solPadding), ' ') << "║\n";
    
    string detailLine = "Elevation: " + formatDouble(solution.elevation, 1) + 
                       "° Azimuth: " + formatDouble(solution.azimuth, 1) + 
                       "° Distance: " + formatDouble(solution.distance, 1);
    int detailPadding = 49 - detailLine.length();
    ss << "║ " << detailLine << string(max(0, detailPadding), ' ') << "║\n";
    
    ss << "╚══════════════════════════════════════════════════════════════╝";
    
    enqueueRefresh(ss.str());
}

void ConsoleUI::renderSystemStatus(const Radar& radar, int totalTargets, 
                                  int detectedTargets) {
    stringstream status;
    
    status << "╔══════════════════════════════════════════════════════════════╗\n";
    status << "║                     SYSTEM STATUS (Phase 6)                 ║\n";
    status << "╠══════════════════════════════════════════════════════════════╣\n";
    
    string sweepLine = "Sweep Angle: " + formatDouble(radar.getCurrentSweepAngle(), 1) + 
                      "° Speed: " + formatDouble(sweepSpeed, 1) + "°/sec";
    int sweepPadding = 49 - sweepLine.length();
    status << "║ " << sweepLine << string(max(0, sweepPadding), ' ') << "║\n";
    
    string frameLine = "Frame Stack: " + to_string(stackSize) + 
                      "/" + to_string(MAX_FRAME_STACK) + 
                      " FPS: " + formatDouble(frameRate, 1);
    int framePadding = 49 - frameLine.length();
    status << "║ " << frameLine << string(max(0, framePadding), ' ') << "║\n";
    
    string targetLine = "Targets Tracked: " + to_string(detectedTargets) + 
                       "/" + to_string(totalTargets);
    int targetPadding = 49 - targetLine.length();
    status << "║ " << targetLine << string(max(0, targetPadding), ' ') << "║\n";
    
    string stackInfo = "Stack Depth: " + to_string(stackSize) + 
                      " (Press 'U' to undo)";
    int stackPadding = 49 - stackInfo.length();
    status << "║ " << stackInfo << string(max(0, stackPadding), ' ') << "║\n";
    
    status << "╚══════════════════════════════════════════════════════════════╝";
    
    enqueueRefresh(status.str());
}

void ConsoleUI::updateFrameRate() {
    auto currentTime = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = currentTime - lastFrameTime;
    
    frameCount++;
    
    // Update frame rate every second
    if (elapsed.count() >= 1.0) {
        frameRate = frameCount / elapsed.count();
        frameCount = 0;
        lastFrameTime = currentTime;
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
    ss << fixed << setprecision(precision) << value;
    return ss.str();
}