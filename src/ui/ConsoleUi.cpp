#include "../../include/ui/ConsoleUI.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <windows.h>
#include <random>
#include <ctime>

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
      frameRate(0.0), frameCount(0), sweepSpeed(45.0),
      // ========== PHASE 7 INIT ==========
      mouseControlEnabled(false),
      mouseControlledTargetId(""),
      mouseCursorX(GRID_WIDTH / 2),
      mouseCursorY(GRID_HEIGHT / 2),
      showMouseCursor(true)
      // ========== END PHASE 7 ==========
{
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
    
    // ========== PHASE 7: INITIALIZE MOUSE ==========
    if (!mouseInput.initialize()) {
        displayMessage("Mouse input initialization failed. Using keyboard fallback.");
    }
    // ========== END PHASE 7 ==========
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

// Copy one grid to another
void ConsoleUI::copyGrid(char dest[GRID_HEIGHT][GRID_WIDTH], const char src[GRID_HEIGHT][GRID_WIDTH]) {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            dest[y][x] = src[y][x];
        }
    }
}

// ========== PHASE 7: MOUSE CONTROL METHODS ==========
void ConsoleUI::updateMouseControl(std::vector<Target>& targets, const Radar& radar) {
    // Poll mouse input
    handleMouseEvents();
    
    // Find or create mouse-controlled target
    bool mouseTargetFound = false;
    static double currentTime = 0.0;
    
    for (auto& target : targets) {
        if (target.getId() == mouseControlledTargetId) {
            mouseTargetFound = true;
            
            // Update target position based on mouse
            Vector2D mouseWorldPos = getMouseWorldPosition(radar);
            target.setPosition(mouseWorldPos);
            
            // Update kinematics with current time
            currentTime += 0.1;
            target.calculateKinematics(currentTime);
            break;
        }
    }
    
    // Create new mouse target if needed
    if (!mouseTargetFound && mouseControlEnabled) {
        mouseControlledTargetId = "MOUSE-" + to_string(time(nullptr));
        Vector2D mouseWorldPos = getMouseWorldPosition(radar);
        
        Target mouseTarget(mouseWorldPos, 800.0, TargetType::UNKNOWN, mouseControlledTargetId);
        targets.push_back(mouseTarget);
        
        displayMessage("Mouse-controlled target created: " + mouseControlledTargetId);
    }
}

void ConsoleUI::handleMouseEvents() {
    // Poll mouse input
    mouseInput.pollEvents();
    
    // Update cursor position from mouse input (if active)
    if (mouseInput.isActive()) {
        mouseCursorX = mouseInput.getMouseX() % GRID_WIDTH;
        mouseCursorY = mouseInput.getMouseY() % GRID_HEIGHT;
    }
}

Vector2D ConsoleUI::getMouseWorldPosition(const Radar& radar) const {
    // Convert screen coordinates to world coordinates
    double radarRange = radar.getRange();
    double worldX = (mouseCursorX - GRID_WIDTH / 2.0) * (radarRange * 2.0 / GRID_WIDTH);
    double worldY = (GRID_HEIGHT / 2.0 - mouseCursorY) * (radarRange * 2.0 / GRID_HEIGHT);
    
    return Vector2D(worldX, worldY);
}

void ConsoleUI::drawMouseCursor(char grid[GRID_HEIGHT][GRID_WIDTH], const Radar& radar) {
    if (!mouseControlEnabled) return;
    
    // Draw mouse cursor at current position
    int cursorX = mouseCursorX;
    int cursorY = mouseCursorY;
    
    // Clamp to grid bounds
    cursorX = max(0, min(GRID_WIDTH - 1, cursorX));
    cursorY = max(0, min(GRID_HEIGHT - 1, cursorY));
    
    // Only draw if the cell is empty or can be overwritten
    char current = grid[cursorY][cursorX];
    if (current == ' ' || current == '.' || current == ':' || current == '*') {
        grid[cursorY][cursorX] = 'M';
    }
    
    // Draw simple crosshair if space permits
    if (cursorY > 0 && grid[cursorY-1][cursorX] == ' ') {
        grid[cursorY-1][cursorX] = '|';
    }
    if (cursorY < GRID_HEIGHT-1 && grid[cursorY+1][cursorX] == ' ') {
        grid[cursorY+1][cursorX] = '|';
    }
    if (cursorX > 0 && grid[cursorY][cursorX-1] == ' ') {
        grid[cursorY][cursorX-1] = '-';
    }
    if (cursorX < GRID_WIDTH-1 && grid[cursorY][cursorX+1] == ' ') {
        grid[cursorY][cursorX+1] = '-';
    }
}
// ========== END PHASE 7 METHODS ==========

// ========== MANUAL STACK IMPLEMENTATION ==========
void ConsoleUI::pushFrame(const char grid[GRID_HEIGHT][GRID_WIDTH], double sweepAngle) {
    auto now = chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    double timestamp = chrono::duration<double>(duration).count();
    
    FrameNode* newNode = new FrameNode(grid, sweepAngle, timestamp);
    
    if (stackSize >= MAX_FRAME_STACK) {
        if (stackTop != nullptr && stackTop->next != nullptr) {
            FrameNode* current = stackTop;
            FrameNode* prev = nullptr;
            
            while (current->next != nullptr) {
                prev = current;
                current = current->next;
            }
            
            if (prev != nullptr) {
                delete prev->next;
                prev->next = nullptr;
                stackSize--;
            }
        } else if (stackTop != nullptr) {
            delete stackTop;
            stackTop = nullptr;
            stackSize = 0;
        }
    }
    
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
    if (stackSize <= 1) {
        stringstream msg;
        msg << "==============================================================\n";
        msg << "               CANNOT UNDO - Need more frames                 \n";
        msg << "==============================================================";
        enqueueRefresh(msg.str());
        return false;
    }
    
    if (!popFrame()) {
        return false;
    }
    
    FrameNode* prevFrame = peekFrame();
    if (prevFrame == nullptr) {
        return false;
    }
    
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            radarGrid[y][x] = prevFrame->grid[y][x];
        }
    }
    
    stringstream msg;
    msg << "==============================================================\n";
    msg << "               FRAME UNDO - Sweep: " 
        << setw(6) << setprecision(1) << fixed << prevFrame->sweepAngle << " deg            \n";
    msg << "==============================================================";
    enqueueRefresh(msg.str());
    
    return true;
}

// ========== ANIMATION METHODS ==========
bool ConsoleUI::shouldAdvanceSweep() {
    if (sweepSpeed <= 0.0) {
        return false;
    }
    
    auto currentTime = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = currentTime - lastSweepTime;
    
    double timePerDegree = 1.0 / sweepSpeed;
    
    if (elapsed.count() >= timePerDegree) {
        lastSweepTime = currentTime;
        return true;
    }
    
    return false;
}

void ConsoleUI::worldToGrid(const Vector2D& worldPos, const Radar& radar, 
                           int& gridX, int& gridY) const {
    double radarRange = radar.getRange();
    Vector2D radarPos = radar.getPosition();
    
    double normalizedX = (worldPos.x - radarPos.x) / radarRange;
    double normalizedY = (worldPos.y - radarPos.y) / radarRange;
    
    gridX = static_cast<int>((normalizedX + 1.0) * (GRID_WIDTH - 1) / 2.0);
    gridY = static_cast<int>((1.0 - normalizedY) * (GRID_HEIGHT - 1) / 2.0);
    
    gridX = max(0, min(GRID_WIDTH - 1, gridX));
    gridY = max(0, min(GRID_HEIGHT - 1, gridY));
}

void ConsoleUI::drawRadarCircle(char grid[GRID_HEIGHT][GRID_WIDTH]) {
    int centerX = GRID_WIDTH / 2;
    int centerY = GRID_HEIGHT / 2;
    
    int radius = min(centerX, centerY) - 1;
    
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
    
    grid[centerY][centerX] = '+';
    if (centerY > 0) grid[centerY - 1][centerX] = '|';
    if (centerY < GRID_HEIGHT - 1) grid[centerY + 1][centerX] = '|';
    if (centerX > 0) grid[centerY][centerX - 1] = '-';
    if (centerX < GRID_WIDTH - 1) grid[centerY][centerX + 1] = '-';
}

void ConsoleUI::drawCompass(char grid[GRID_HEIGHT][GRID_WIDTH]) {
    if (GRID_WIDTH / 2 >= 0 && GRID_WIDTH / 2 < GRID_WIDTH) {
        grid[0][GRID_WIDTH / 2] = 'N';
    }
    
    if (GRID_WIDTH / 2 >= 0 && GRID_WIDTH / 2 < GRID_WIDTH && 
        GRID_HEIGHT - 1 >= 0 && GRID_HEIGHT - 1 < GRID_HEIGHT) {
        grid[GRID_HEIGHT - 1][GRID_WIDTH / 2] = 'S';
    }
    
    if (GRID_WIDTH - 1 >= 0 && GRID_WIDTH - 1 < GRID_WIDTH && 
        GRID_HEIGHT / 2 >= 0 && GRID_HEIGHT / 2 < GRID_HEIGHT) {
        grid[GRID_HEIGHT / 2][GRID_WIDTH - 1] = 'E';
    }
    
    if (0 >= 0 && 0 < GRID_WIDTH && 
        GRID_HEIGHT / 2 >= 0 && GRID_HEIGHT / 2 < GRID_HEIGHT) {
        grid[GRID_HEIGHT / 2][0] = 'W';
    }
}

void ConsoleUI::drawSweepLine(char grid[GRID_HEIGHT][GRID_WIDTH], double angle, const Radar& radar) {
    int centerX = GRID_WIDTH / 2;
    int centerY = GRID_HEIGHT / 2;
    int radius = min(centerX, centerY) - 1;
    
    double rad = angle * M_PI / 180.0;
    
    static int prevSweepPositions[100][2];
    static int prevSweepCount = 0;
    
    for (int i = 0; i < prevSweepCount; i++) {
        int x = prevSweepPositions[i][0];
        int y = prevSweepPositions[i][1];
        
        if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
            grid[y][x] = prevRadarGrid[y][x];
        }
    }
    
    prevSweepCount = 0;
    
    for (int r = 1; r <= radius && prevSweepCount < 100; r++) {
        int x = centerX + static_cast<int>(r * cos(rad));
        int y = centerY - static_cast<int>(r * sin(rad));
        
        if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
            prevSweepPositions[prevSweepCount][0] = x;
            prevSweepPositions[prevSweepCount][1] = y;
            prevSweepCount++;
            
            char sweepChar;
            int pattern = r % 3;
            if (pattern == 0) sweepChar = '\\';
            else if (pattern == 1) sweepChar = '/';
            else sweepChar = '|';
            
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
            continue;
        }
        
        int gridX, gridY;
        worldToGrid(target.getPosition(), radar, gridX, gridY);
        
        char symbol;
        switch (target.getType()) {
            case TargetType::FRIENDLY:
                symbol = 'F';
                break;
            case TargetType::UNKNOWN:
            default:
                symbol = 'X';
                break;
        }
        
        // ========== PHASE 7: SPECIAL SYMBOL FOR MOUSE-CONTROLLED TARGET ==========
        if (target.getId().find("MOUSE") != string::npos) {
            symbol = 'O'; // Use 'O' instead of Unicode '◎'
        }
        // ========== END PHASE 7 ==========
        
        if (gridX >= 0 && gridX < GRID_WIDTH && gridY >= 0 && gridY < GRID_HEIGHT) {
            grid[gridY][gridX] = symbol;
        }
    }
}

void ConsoleUI::drawHUD(const Radar& radar, const vector<Target>& targets) {
    int unknownCount = 0;
    int friendlyCount = 0;
    int inRangeCount = 0;
    
    for (const Target& target : targets) {
        if (target.getType() == TargetType::UNKNOWN) unknownCount++;
        else if (target.getType() == TargetType::FRIENDLY) friendlyCount++;
        
        if (radar.isInRange(target)) inRangeCount++;
    }
    
    stringstream hud;
    hud << fixed << setprecision(1);
    
    hud << "==============================================================\n";
    hud << "                    AIR DEFENSE RADAR SYSTEM                  \n";
    hud << "==============================================================\n";
    
    string status = (inRangeCount > 0) ? "ACTIVE TRACKING" : "SCANNING";
    int statusPadding = 49 - status.length();
    hud << " Status: " << status << string(max(0, statusPadding), ' ') << "\n";
    
    string radarInfo = "Radar Position: (" + to_string(static_cast<int>(radar.getPosition().x)) + 
                       ", " + to_string(static_cast<int>(radar.getPosition().y)) + 
                       ") Range: " + to_string(static_cast<int>(radar.getRange()));
    int radarPadding = 49 - radarInfo.length();
    hud << " " << radarInfo << string(max(0, radarPadding), ' ') << "\n";
    
    string targetInfo = "Targets - Total: " + to_string(targets.size()) + 
                        " In Range: " + to_string(inRangeCount) + 
                        " Unknown: " + to_string(unknownCount) + 
                        " Friendly: " + to_string(friendlyCount);
    int targetPadding = 49 - targetInfo.length();
    hud << " " << targetInfo << string(max(0, targetPadding), ' ') << "\n";
    
    // ========== PHASE 7: ADD MOUSE STATUS ==========
    string mouseStatus = "Mouse Control: " + string(mouseControlEnabled ? "ENABLED" : "DISABLED");
    if (mouseControlEnabled && !mouseControlledTargetId.empty()) {
        mouseStatus += " | Target: " + mouseControlledTargetId.substr(0, 8) + "...";
    }
    int mousePadding = 49 - mouseStatus.length();
    hud << " " << mouseStatus << string(max(0, mousePadding), ' ') << "\n";
    // ========== END PHASE 7 ==========
    
    string fpsInfo = "Frame Rate: " + formatDouble(frameRate, 1) + " fps";
    int fpsPadding = 49 - fpsInfo.length();
    hud << " " << fpsInfo << string(max(0, fpsPadding), ' ') << "\n";
    
    hud << "==============================================================";
    
    enqueueRefresh(hud.str());
}

void ConsoleUI::enqueueRefresh(const string& message) {
    if (isRefreshQueueFull()) {
        dequeueRefresh();
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

// UPDATED VERSION - Now includes mouse control
void ConsoleUI::renderRadarDisplay(const Radar& radar, vector<Target>& targets, bool autoSweep) {
    // Store previous frame before changes
    copyGrid(prevRadarGrid, radarGrid);
    
    // Clear the current grid
    clearGrid(radarGrid);
    
    // ========== PHASE 7: UPDATE MOUSE-CONTROLLED TARGET ==========
    if (mouseControlEnabled) {
        updateMouseControl(targets, radar);
    }
    // ========== END PHASE 7 ==========
    
    // Draw radar elements
    drawRadarCircle(radarGrid);
    drawCompass(radarGrid);
    
    // Get current sweep angle
    double sweepAngle = radar.getCurrentSweepAngle();
    
    // Draw sweep line with animation
    drawSweepLine(radarGrid, sweepAngle, radar);
    
    // Draw targets
    drawTargets(radarGrid, targets, radar);
    
    // ========== PHASE 7: DRAW MOUSE CURSOR ==========
    drawMouseCursor(radarGrid, radar);
    // ========== END PHASE 7 ==========
    
    // Push current frame to manual stack for undo functionality
    pushFrame(radarGrid, sweepAngle);
    
    // Print the radar grid
    clearScreen();
    
    cout << "==============================================================\n";
    cout << "                   LIVE RADAR DISPLAY (Phase 7)               \n";
    cout << "==============================================================\n";
    
    static int borderAnim = 0;
    borderAnim = (borderAnim + 1) % 60;
    
    for (int y = 0; y < GRID_HEIGHT; y++) {
        char leftBorder = '|';
        char rightBorder = '|';
        
        if ((borderAnim / 10) % 2 == 0) {
            if (y % 3 == 0) {
                leftBorder = '#';
                rightBorder = '#';
            }
        }
        
        cout << leftBorder;
        
        for (int x = 0; x < GRID_WIDTH; x++) {
            char displayChar = radarGrid[y][x];
            
            if (displayChar == '\\' || displayChar == '/' || displayChar == '|') {
                static int pulseCounter = 0;
                pulseCounter++;
                
                if ((pulseCounter / 3) % 2 == 0) {
                    cout << displayChar;
                } else {
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
    
    cout << "--------------------------------------------------------------\n";
    cout << " Stack: " << setw(2) << stackSize << "/" << MAX_FRAME_STACK 
         << " frames | Sweep: " << setw(6) << setprecision(1) << fixed << sweepAngle 
         << " deg | Speed: " << setw(5) << sweepSpeed << " deg/sec" << " \n";
    
    cout << "--------------------------------------------------------------\n";
    if (autoSweep) {
        cout << "\033[32m";  // Green for AUTO mode
        cout << " MODE: AUTO (Sweep advancing automatically)";
    } else {
        cout << "\033[33m";  // Yellow for MANUAL mode
        cout << " MODE: MANUAL (Use LEFT/RIGHT arrows to control sweep)";
    }
    cout << "\033[0m\n";
    
    // ========== PHASE 7: MOUSE CONTROL STATUS ==========
    if (mouseControlEnabled) {
        cout << "\033[35m";  // Magenta for mouse control
        cout << " MOUSE: ACTIVE (Press 'C' to disable | Arrow keys move cursor)";
        cout << "\033[0m\n";
    } else {
        cout << "\033[90m";  // Gray
        cout << " MOUSE: INACTIVE (Press 'C' to control target with cursor)";
        cout << "\033[0m\n";
    }
    // ========== END PHASE 7 ==========
    
    cout << " Controls: SPACE/ENTER=Next | C=Mouse | U=undo | +/-=speed | ESC=exit\n";
    cout << "==============================================================\n";
    
    string animBar = "[";
    int barLength = 20;
    int filled = (borderAnim % barLength);
    for (int i = 0; i < barLength; i++) {
        if (i <= filled) animBar += "=";
        else animBar += ".";
    }
    animBar += "]";
    
    if (autoSweep) {
        cout << " Auto Sweep Progress: " << animBar << "\n";
    }
    
    drawHUD(radar, targets);
    
    while (!isRefreshQueueEmpty()) {
        string msg = dequeueRefresh();
        if (!autoSweep && msg.find("SWEEP ADVANCING") != string::npos) {
            continue;
        }
        cout << msg << "\n";
    }
    
    updateFrameRate();
    
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
    
    double horizontalDist, displacement, bearing;
    string direction;
    radar.analyzeTarget(target, horizontalDist, displacement, bearing, direction);
    
    info << fixed << setprecision(1);
    info << "==============================================================\n";
    info << "                     TARGET INFORMATION                       \n";
    info << "--------------------------------------------------------------\n";
    
    string targetIdLine = "ID: " + target.getId() + 
                         " Type: " + (target.getType() == TargetType::UNKNOWN ? "UNKNOWN" : "FRIENDLY");
    int idPadding = 49 - targetIdLine.length();
    info << " " << targetIdLine << string(max(0, idPadding), ' ') << "\n";
    
    string posLine = "Position: (" + to_string(static_cast<int>(target.getPosition().x)) + 
                     ", " + to_string(static_cast<int>(target.getPosition().y)) + 
                     ") Height: " + to_string(static_cast<int>(target.getHeight()));
    int posPadding = 49 - posLine.length();
    info << " " << posLine << string(max(0, posPadding), ' ') << "\n";
    
    string distLine = "Horizontal Distance: " + formatDouble(horizontalDist, 1) + 
                     " Displacement: " + formatDouble(displacement, 1);
    int distPadding = 49 - distLine.length();
    info << " " << distLine << string(max(0, distPadding), ' ') << "\n";
    
    string bearingLine = "Bearing: " + formatDouble(bearing, 1) + 
                        " deg Direction: " + direction;
    int bearingPadding = 49 - bearingLine.length();
    info << " " << bearingLine << string(max(0, bearingPadding), ' ') << "\n";
    
    string speedLine = "Speed: " + formatDouble(target.getSpeed(), 1) + 
                      " Velocity: (" + formatDouble(target.getVelocity().x, 1) + 
                      ", " + formatDouble(target.getVelocity().y, 1) + ")";
    int speedPadding = 49 - speedLine.length();
    info << " " << speedLine << string(max(0, speedPadding), ' ') << "\n";
    
    // ========== PHASE 7: MARK MOUSE-CONTROLLED TARGET ==========
    if (target.getId() == mouseControlledTargetId) {
        info << "--------------------------------------------------------------\n";
        info << "                *** MOUSE-CONTROLLED TARGET ***               \n";
    }
    // ========== END PHASE 7 ==========
    
    info << "==============================================================";
    
    enqueueRefresh(info.str());
}

void ConsoleUI::renderFiringSolution(const Gun& gun, const Target& target) {
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
    
    // ========== PHASE 7: MARK MOUSE-CONTROLLED TARGET FIRING SOLUTION ==========
    if (target.getId() == mouseControlledTargetId) {
        ss << "--------------------------------------------------------------\n";
        ss << "             *** TARGET CONTROLLED BY MOUSE ***               \n";
        ss << "          (Move mouse to adjust firing solution)              \n";
    }
    // ========== END PHASE 7 ==========
    
    ss << "==============================================================";
    
    string coloredMsg = "\033[31m" + ss.str() + "\033[0m";
    enqueueRefresh(coloredMsg);
}

void ConsoleUI::renderSystemStatus(const Radar& radar, int totalTargets, 
                                  int detectedTargets) {
    stringstream status;
    
    status << "==============================================================\n";
    status << "                     SYSTEM STATUS (Phase 7)                 \n";
    status << "--------------------------------------------------------------\n";
    
    string sweepLine = "Sweep Angle: " + formatDouble(radar.getCurrentSweepAngle(), 1) + 
                      " deg Speed: " + formatDouble(sweepSpeed, 1) + " deg/sec";
    int sweepPadding = 49 - sweepLine.length();
    status << " " << sweepLine << string(max(0, sweepPadding), ' ') << "\n";
    
    string frameLine = "Frame Stack: " + to_string(stackSize) + 
                      "/" + to_string(MAX_FRAME_STACK) + 
                      " FPS: " + formatDouble(frameRate, 1);
    int framePadding = 49 - frameLine.length();
    status << " " << frameLine << string(max(0, framePadding), ' ') << "\n";
    
    string targetLine = "Targets Tracked: " + to_string(detectedTargets) + 
                       "/" + to_string(totalTargets);
    int targetPadding = 49 - targetLine.length();
    status << " " << targetLine << string(max(0, targetPadding), ' ') << "\n";
    
    // ========== PHASE 7: MOUSE CONTROL STATUS ==========
    string mouseLine = "Mouse Control: " + string(mouseControlEnabled ? "ENABLED" : "DISABLED");
    if (mouseControlEnabled) {
        mouseLine += " | Press 'C' to toggle";
    }
    int mousePadding = 49 - mouseLine.length();
    status << " " << mouseLine << string(max(0, mousePadding), ' ') << "\n";
    // ========== END PHASE 7 ==========
    
    status << "==============================================================";
    
    enqueueRefresh(status.str());
}

void ConsoleUI::updateFrameRate() {
    auto currentTime = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = currentTime - lastFrameTime;
    
    frameCount++;
    
    if (elapsed.count() >= 1.0) {
        frameRate = frameCount / elapsed.count();
        frameCount = 0;
        lastFrameTime = currentTime;
    }
}

void ConsoleUI::clearScreen() {
    cout << "\033[2J\033[1;1H";
}

void ConsoleUI::setCursorPosition(int x, int y) {
    cout << "\033[" << y + 1 << ";" << x + 1 << "H";
}

string ConsoleUI::formatDouble(double value, int precision) {
    stringstream ss;
    ss << fixed << setprecision(precision) << value;
    return ss.str();
}