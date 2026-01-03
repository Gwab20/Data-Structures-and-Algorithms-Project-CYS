#ifndef MOUSEINPUT_HPP
#define MOUSEINPUT_HPP

#include "../utils/MathUtils.hpp"
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

class MouseInput {
private:
    // Platform-specific state
    #ifdef _WIN32
    HANDLE hConsoleInput;
    DWORD oldConsoleMode;
    #endif
    
    // Mouse state
    int mouseX, mouseY;
    bool leftButtonPressed;
    bool rightButtonPressed;
    bool mouseActive;
    
    // Screen to world conversion
    double screenToWorldScale;
    Vector2D screenCenter;
    
public:
    MouseInput();
    ~MouseInput();
    
    // Initialize mouse input
    bool initialize();
    
    // Poll for mouse events
    void pollEvents();
    
    // Get mouse state
    int getMouseX() const { return mouseX; }
    int getMouseY() const { return mouseY; }
    bool isLeftPressed() const { return leftButtonPressed; }
    bool isRightPressed() const { return rightButtonPressed; }
    bool isActive() const { return mouseActive; }
    
    // Convert screen coordinates to world coordinates
    Vector2D screenToWorld(int screenX, int screenY, double radarRange) const;
    
    // Convert world coordinates to screen coordinates
    void worldToScreen(const Vector2D& worldPos, double radarRange, int& screenX, int& screenY) const;
    
    // Set conversion parameters
    void setScreenCenter(int centerX, int centerY) { 
        screenCenter = Vector2D(centerX, centerY); 
    }
    void setScale(double scale) { 
        screenToWorldScale = scale; 
    }
    
    // Platform-specific implementations
    #ifdef _WIN32
    bool initializeWindows();
    void pollEventsWindows();
    #else
    bool initializeUnix();
    void pollEventsUnix();
    #endif
};

#endif // MOUSEINPUT_HPP