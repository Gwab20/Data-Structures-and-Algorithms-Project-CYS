#ifndef CONSOLEUI_HPP
#define CONSOLEUI_HPP

#include "../radar/Radar.hpp"
#include "../radar/Target.hpp"
#include <vector>
#include <string>
#include <chrono>

// Stack node for storing previous frames
struct FrameNode {
    char grid[21][61];  // GRID_HEIGHT x GRID_WIDTH
    double sweepAngle;
    double timestamp;
    FrameNode* next;  // For linked list stack
    
    FrameNode(const char gridCopy[21][61], double angle, double time);
};

class ConsoleUI {
private:
    // Grid dimensions for ASCII radar
    static const int GRID_WIDTH = 61;
    static const int GRID_HEIGHT = 21;
    
    // ASCII grid for radar display - double buffering
    char radarGrid[GRID_HEIGHT][GRID_WIDTH];
    char prevRadarGrid[GRID_HEIGHT][GRID_WIDTH];
    
    // Manual stack implementation for frame undo functionality
    FrameNode* stackTop;
    int stackSize;
    static const int MAX_FRAME_STACK = 10;
    
    // Screen refresh management
    static const int MAX_REFRESH_QUEUE = 20;
    std::string refreshQueue[MAX_REFRESH_QUEUE];
    int refreshFront;
    int refreshRear;
    int refreshCount;
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point lastFrameTime;
    std::chrono::high_resolution_clock::time_point lastSweepTime;
    double frameRate;
    int frameCount;
    double sweepSpeed;  // Degrees per second
    
    // Colors for different target types
    enum Color {
        COLOR_UNKNOWN = 1,
        COLOR_FRIENDLY = 2,
        COLOR_RADAR = 3,
        COLOR_SWEEP = 4,
        COLOR_INFO = 5
    };
    
    // Helper methods
    void initColors();
    void clearGrid(char grid[GRID_HEIGHT][GRID_WIDTH]);
    void copyGrid(char dest[GRID_HEIGHT][GRID_WIDTH], const char src[GRID_HEIGHT][GRID_WIDTH]);
    void drawRadarCircle(char grid[GRID_HEIGHT][GRID_WIDTH]);
    void drawCompass(char grid[GRID_HEIGHT][GRID_WIDTH]);
    void drawSweepLine(char grid[GRID_HEIGHT][GRID_WIDTH], double angle, const Radar& radar);
    void drawTargets(char grid[GRID_HEIGHT][GRID_WIDTH], const std::vector<Target>& targets, const Radar& radar);
    void drawHUD(const Radar& radar, const std::vector<Target>& targets);
    
    // Manual stack operations
    void pushFrame(const char grid[GRID_HEIGHT][GRID_WIDTH], double sweepAngle);
    bool popFrame();
    FrameNode* peekFrame() const;
    void clearStack();
    
    // Queue operations for screen refresh
    void enqueueRefresh(const std::string& message);
    std::string dequeueRefresh();
    bool isRefreshQueueEmpty() const;
    bool isRefreshQueueFull() const;
    
    // Coordinate mapping
    void worldToGrid(const Vector2D& worldPos, const Radar& radar, 
                     int& gridX, int& gridY) const;
    
public:
    ConsoleUI();
    ~ConsoleUI();
    
    // Main rendering methods - UPDATED to include mode
    void renderRadarDisplay(const Radar& radar, 
                           const std::vector<Target>& targets,
                           bool autoSweep = false);  // Added autoSweep parameter
    
    void renderTargetInfo(const Target& target, const Radar& radar);
    void renderFiringSolution(const Gun& gun, const Target& target);
    void renderSystemStatus(const Radar& radar, int totalTargets, 
                           int detectedTargets);
    
    // Animation control
    bool shouldAdvanceSweep();
    void setSweepSpeed(double degreesPerSec) { sweepSpeed = degreesPerSec; }
    double getSweepSpeed() const { return sweepSpeed; }
    
    // Manual stack operations (public interface)
    bool undoLastFrame();
    bool canUndo() const { return stackSize > 1; }  // Need at least 2 frames to undo
    int getStackSize() const { return stackSize; }
    
    // Update methods
    void updateFrameRate();
    double getFrameRate() const { return frameRate; }
    
    // Clear screen (platform independent)
    static void clearScreen();
    
    // Set cursor position for terminal output
    static void setCursorPosition(int x, int y);
    
    // Utility for formatted output
    static std::string formatDouble(double value, int precision = 1);
    
    // Public interface for adding messages to refresh queue
    void displayMessage(const std::string& message) {
        enqueueRefresh(message);
    }
};

// FrameNode constructor implementation
inline FrameNode::FrameNode(const char gridCopy[21][61], double angle, double time) 
    : sweepAngle(angle), timestamp(time), next(nullptr) {
    // Copy grid data
    for (int y = 0; y < 21; y++) {
        for (int x = 0; x < 61; x++) {
            grid[y][x] = gridCopy[y][x];
        }
    }
}

#endif