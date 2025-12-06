#ifndef CONSOLEUI_HPP
#define CONSOLEUI_HPP

#include "../radar/Radar.hpp"
#include "../radar/Target.hpp"
#include <vector>
#include <string>
#include <chrono>

class ConsoleUI {
private:
    // Grid dimensions for ASCII radar
    static const int GRID_WIDTH = 61;
    static const int GRID_HEIGHT = 21;
    
    // ASCII grid for radar display
    char radarGrid[GRID_HEIGHT][GRID_WIDTH];
    
    // Screen refresh management
    static const int MAX_REFRESH_QUEUE = 20;
    std::string refreshQueue[MAX_REFRESH_QUEUE];
    int refreshFront;
    int refreshRear;
    int refreshCount;
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point lastFrameTime;
    double frameRate;
    int frameCount;
    
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
    void clearGrid();
    void drawRadarCircle();
    void drawCompass();
    void drawSweepLine(double angle, const Radar& radar);
    void drawTargets(const std::vector<Target>& targets, const Radar& radar);
    void drawHUD(const Radar& radar, const std::vector<Target>& targets);
    
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
                           const std::vector<Target>& targets);
    
    void renderTargetInfo(const Target& target, const Radar& radar);
    void renderFiringSolution(const Gun& gun, const Target& target);
    void renderSystemStatus(const Radar& radar, int totalTargets, 
                           int detectedTargets);
    
    // Update methods
    void updateFrameRate();
    double getFrameRate() const { return frameRate; }
    
    // Clear screen (platform independent)
    static void clearScreen();
    
    // Set cursor position for terminal output
    static void setCursorPosition(int x, int y);
    
    // Utility for formatted output
    static std::string formatDouble(double value, int precision = 1);
};

#endif