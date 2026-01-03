#ifndef CONSOLEUI_HPP
#define CONSOLEUI_HPP

#include "../radar/Radar.hpp"
#include "../radar/Target.hpp"
#include "../ui/MouseInput.hpp"
#include <vector>
#include <string>
#include <chrono>

// Stack node for storing previous frames
struct FrameNode {
    char grid[21][61];  // GRID_HEIGHT x GRID_WIDTH
    double sweepAngle; 
    double timestamp; // when this frame was shown
    FrameNode* next;  // For linked list stack
    
    FrameNode(const char gridCopy[21][61], double angle, double time);
};

class ConsoleUI {
private:
    // Grid dimensions for ASCII radar
    static const int GRID_WIDTH = 61;
    static const int GRID_HEIGHT = 21;
    
    // ASCII grid for radar display - double buffering
    char radarGrid[GRID_HEIGHT][GRID_WIDTH]; // current frame
    char prevRadarGrid[GRID_HEIGHT][GRID_WIDTH]; // previous frame
     
    // Manual stack implementation for frame undo functionality
    FrameNode* stackTop; 
    int stackSize; // how many frames are stored
    static const int MAX_FRAME_STACK = 10; // max frames to remember
    
    // Screen refresh management
    static const int MAX_REFRESH_QUEUE = 20;
    std::string refreshQueue[MAX_REFRESH_QUEUE];
    int refreshFront; // where to take messages from
    int refreshRear; // where to add messages
    int refreshCount; // how many messages are waiting
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point lastFrameTime;
    std::chrono::high_resolution_clock::time_point lastSweepTime;
    double frameRate; // FPS counter
    int frameCount; // frame counter
    double sweepSpeed;  // Degrees per second
    
    // ========== PHASE 7: MOUSE CONTROL ADDITIONS ==========
    MouseInput mouseInput;
    bool mouseControlEnabled;
    std::string mouseControlledTargetId;
    int mouseCursorX, mouseCursorY;
    bool showMouseCursor;
    // ========== END PHASE 7 ADDITIONS ==========
    
    // Colors for different target types
    enum Color {
        COLOR_UNKNOWN = 1,
        COLOR_FRIENDLY = 2,
        COLOR_RADAR = 3,
        COLOR_SWEEP = 4,
        COLOR_INFO = 5,
        COLOR_MOUSE = 6
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
    
    // ========== PHASE 7: MOUSE METHODS ==========
    void drawMouseCursor(char grid[GRID_HEIGHT][GRID_WIDTH], const Radar& radar);
    void updateMouseControl(std::vector<Target>& targets, const Radar& radar);
    Vector2D getMouseWorldPosition(const Radar& radar) const;
    void handleMouseEvents();
    // ========== END PHASE 7 METHODS ==========
    
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
    
    // Main rendering methods
    void renderRadarDisplay(const Radar& radar, 
                           std::vector<Target>& targets,
                           bool autoSweep = false);
    
    void renderTargetInfo(const Target& target, const Radar& radar);
    void renderFiringSolution(const Gun& gun, const Target& target);
    void renderSystemStatus(const Radar& radar, int totalTargets, 
                           int detectedTargets);
    
    // Animation control
    bool shouldAdvanceSweep();
    void setSweepSpeed(double degreesPerSec) { sweepSpeed = degreesPerSec; }
    double getSweepSpeed() const { return sweepSpeed; }
    
    // Manual stack operations
    bool undoLastFrame();
    bool canUndo() const { return stackSize > 1; }
    int getStackSize() const { return stackSize; }
    
    // Update methods
    void updateFrameRate();
    double getFrameRate() const { return frameRate; }
    
    // ========== PHASE 7: MOUSE CONTROL PUBLIC INTERFACE ==========
    bool isMouseControlEnabled() const { return mouseControlEnabled; }
    void toggleMouseControl(bool enable) { 
        mouseControlEnabled = enable; 
        if (enable) {
            enqueueRefresh("=== MOUSE CONTROL ENABLED ===");
            enqueueRefresh("Mouse cursor: M | Arrow keys move cursor");
        } else {
            enqueueRefresh("Mouse control disabled");
        }
    }
    
    void moveMouseCursor(int deltaX, int deltaY) {
        mouseCursorX += deltaX;
        mouseCursorY += deltaY;
        
        // Clamp to grid bounds
        mouseCursorX = std::max(0, std::min(GRID_WIDTH - 1, mouseCursorX));
        mouseCursorY = std::max(0, std::min(GRID_HEIGHT - 1, mouseCursorY));
    }
    
    void setMouseClick(bool clicked) {
        if (clicked && !mouseControlEnabled) {
            toggleMouseControl(true);
        }
    }
    // ========== END PHASE 7 PUBLIC INTERFACE ==========
    
    // Clear screen
    static void clearScreen();
    
    // Set cursor position
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