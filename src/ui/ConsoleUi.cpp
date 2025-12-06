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
    : refreshFront(0), refreshRear(0), refreshCount(0),
      frameRate(0.0), frameCount(0) {
    
    // Initialize grid with empty spaces
    clearGrid();
    
    // Initialize refresh queue
    for (int i = 0; i < MAX_REFRESH_QUEUE; i++) {
        refreshQueue[i] = "";
    }
    
    // Initialize colors if on Windows
    initColors();
    
    lastFrameTime = chrono::high_resolution_clock::now();
}

ConsoleUI::~ConsoleUI() {
    // Nothing to clean up
}

void ConsoleUI::initColors() {
    // Windows console color setup
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Define color pairs
    // Unknown: Red on black (4), Friendly: Green (2), Radar: Cyan (3), Sweep: Yellow (6)
    // This is just setup - actual color application would need more implementation
}

void ConsoleUI::clearGrid() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            radarGrid[y][x] = ' ';
        }
    }
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

void ConsoleUI::drawRadarCircle() {
    // Draw radar circle (concentric circles)
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
                if (r == 1) radarGrid[y][x] = '.';
                else if (r == 2) radarGrid[y][x] = ':';
                else radarGrid[y][x] = '*';
            }
        }
    }
    
    // Draw center crosshair
    radarGrid[centerY][centerX] = '+';
    if (centerY > 0) radarGrid[centerY - 1][centerX] = '|';
    if (centerY < GRID_HEIGHT - 1) radarGrid[centerY + 1][centerX] = '|';
    if (centerX > 0) radarGrid[centerY][centerX - 1] = '-';
    if (centerX < GRID_WIDTH - 1) radarGrid[centerY][centerX + 1] = '-';
}

void ConsoleUI::drawCompass() {
    // Draw compass directions at edges
    // North
    if (GRID_WIDTH / 2 >= 0 && GRID_WIDTH / 2 < GRID_WIDTH) {
        radarGrid[0][GRID_WIDTH / 2] = 'N';
    }
    
    // South
    if (GRID_WIDTH / 2 >= 0 && GRID_WIDTH / 2 < GRID_WIDTH && 
        GRID_HEIGHT - 1 >= 0 && GRID_HEIGHT - 1 < GRID_HEIGHT) {
        radarGrid[GRID_HEIGHT - 1][GRID_WIDTH / 2] = 'S';
    }
    
    // East
    if (GRID_WIDTH - 1 >= 0 && GRID_WIDTH - 1 < GRID_WIDTH && 
        GRID_HEIGHT / 2 >= 0 && GRID_HEIGHT / 2 < GRID_HEIGHT) {
        radarGrid[GRID_HEIGHT / 2][GRID_WIDTH - 1] = 'E';
    }
    
    // West
    if (0 >= 0 && 0 < GRID_WIDTH && 
        GRID_HEIGHT / 2 >= 0 && GRID_HEIGHT / 2 < GRID_HEIGHT) {
        radarGrid[GRID_HEIGHT / 2][0] = 'W';
    }
    
    // Intermediate directions (NE, NW, SE, SW)
    if (GRID_WIDTH - 3 >= 0 && GRID_WIDTH - 3 < GRID_WIDTH && 
        GRID_WIDTH - 2 >= 0 && GRID_WIDTH - 2 < GRID_WIDTH &&
        2 >= 0 && 2 < GRID_HEIGHT) {
        radarGrid[2][GRID_WIDTH - 3] = 'N';
        radarGrid[2][GRID_WIDTH - 2] = 'E';
    }
    
    if (2 >= 0 && 2 < GRID_WIDTH && 
        1 >= 0 && 1 < GRID_WIDTH &&
        2 >= 0 && 2 < GRID_HEIGHT) {
        radarGrid[2][2] = 'N';
        radarGrid[2][1] = 'W';
    }
    
    if (2 >= 0 && 2 < GRID_WIDTH && 
        1 >= 0 && 1 < GRID_WIDTH &&
        GRID_HEIGHT - 3 >= 0 && GRID_HEIGHT - 3 < GRID_HEIGHT) {
        radarGrid[GRID_HEIGHT - 3][2] = 'S';
        radarGrid[GRID_HEIGHT - 3][1] = 'W';
    }
    
    if (GRID_WIDTH - 3 >= 0 && GRID_WIDTH - 3 < GRID_WIDTH && 
        GRID_WIDTH - 2 >= 0 && GRID_WIDTH - 2 < GRID_WIDTH &&
        GRID_HEIGHT - 3 >= 0 && GRID_HEIGHT - 3 < GRID_HEIGHT) {
        radarGrid[GRID_HEIGHT - 3][GRID_WIDTH - 3] = 'S';
        radarGrid[GRID_HEIGHT - 3][GRID_WIDTH - 2] = 'E';
    }
}

void ConsoleUI::drawSweepLine(double angle, const Radar& radar) {
    int centerX = GRID_WIDTH / 2;
    int centerY = GRID_HEIGHT / 2;
    int radius = min(centerX, centerY) - 1;
    
    // Convert angle to radians
    double rad = angle * M_PI / 180.0;
    
    // Draw sweep line using simple ray casting
    for (int r = 1; r <= radius; r++) {
        int x = centerX + static_cast<int>(r * cos(rad));
        int y = centerY - static_cast<int>(r * sin(rad)); // Negative because screen Y increases downward
        
        if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
            // Use different characters for sweep effect
            char sweepChar;
            if (r % 3 == 0) sweepChar = '\\';
            else if (r % 3 == 1) sweepChar = '/';
            else sweepChar = '|';
            
            radarGrid[y][x] = sweepChar;
        }
    }
}

void ConsoleUI::drawTargets(const vector<Target>& targets, const Radar& radar) {
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
            radarGrid[gridY][gridX] = symbol;
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
    // Clear the grid
    clearGrid();
    
    // Draw radar elements
    drawRadarCircle();
    drawCompass();
    drawSweepLine(radar.getCurrentSweepAngle(), radar);
    drawTargets(targets, radar);
    
    // Print the radar grid
    clearScreen();
    
    cout << "╔══════════════════════════════════════════════════════════════╗\n";
    cout << "║                         RADAR DISPLAY                        ║\n";
    cout << "╠══════════════════════════════════════════════════════════════╣\n";
    
    for (int y = 0; y < GRID_HEIGHT; y++) {
        cout << "║";
        for (int x = 0; x < GRID_WIDTH; x++) {
            cout << radarGrid[y][x];
        }
        cout << "║\n";
    }
    
    cout << "╠══════════════════════════════════════════════════════════════╣\n";
    cout << "║ Legend: [R] Radar  [X] Unknown  [F] Friendly  [/|\\] Sweep   ║\n";
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
    status << "║                     SYSTEM STATUS                           ║\n";
    status << "╠══════════════════════════════════════════════════════════════╣\n";
    
    string sweepLine = "Sweep Angle: " + formatDouble(radar.getCurrentSweepAngle(), 1) + "°";
    int sweepPadding = 49 - sweepLine.length();
    status << "║ " << sweepLine << string(max(0, sweepPadding), ' ') << "║\n";
    
    string targetLine = "Targets Tracked: " + to_string(detectedTargets) + 
                       "/" + to_string(totalTargets);
    int targetPadding = 49 - targetLine.length();
    status << "║ " << targetLine << string(max(0, targetPadding), ' ') << "║\n";
    
    string gunLine = "Gun Ready: YES";
    int gunPadding = 49 - gunLine.length();
    status << "║ " << gunLine << string(max(0, gunPadding), ' ') << "║\n";
    
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
    // This works on Windows 10+ with virtual terminal enabled
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