#include "../../include/ui/MouseInput.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

MouseInput::MouseInput() 
    : mouseX(0), mouseY(0), 
      leftButtonPressed(false), 
      rightButtonPressed(false),
      mouseActive(false),
      screenToWorldScale(1.0),
      screenCenter(Vector2D(0, 0)) {
    
    #ifdef _WIN32
    hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
    #endif
}

MouseInput::~MouseInput() {
    #ifdef _WIN32
    // Restore console mode
    if (hConsoleInput) {
        SetConsoleMode(hConsoleInput, oldConsoleMode);
    }
    #endif
}

bool MouseInput::initialize() {
    #ifdef _WIN32
    return initializeWindows();
    #else
    return initializeUnix();
    #endif
}

#ifdef _WIN32
bool MouseInput::initializeWindows() {
    if (!hConsoleInput || hConsoleInput == INVALID_HANDLE_VALUE) {
        cerr << "Failed to get console input handle" << endl;
        return false;
    }
    
    // Get current console mode
    if (!GetConsoleMode(hConsoleInput, &oldConsoleMode)) {
        cerr << "Failed to get console mode" << endl;
        return false;
    }
    
    // Enable mouse input
    DWORD newMode = oldConsoleMode;
    newMode |= ENABLE_MOUSE_INPUT;
    newMode &= ~ENABLE_QUICK_EDIT_MODE; // Disable quick edit
    newMode &= ~ENABLE_PROCESSED_INPUT; // Need this for mouse events
    
    if (!SetConsoleMode(hConsoleInput, newMode)) {
        cerr << "Failed to set console mode for mouse input" << endl;
        return false;
    }
    
    // Get console screen buffer info for dimensions
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        int width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        int height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        screenCenter = Vector2D(width / 2, height / 2);
    }
    
    mouseActive = true;
    return true;
}

void MouseInput::pollEventsWindows() {
    if (!mouseActive || !hConsoleInput) return;
    
    DWORD numEvents = 0;
    DWORD numEventsRead = 0;
    
    // Check if there are events
    if (!GetNumberOfConsoleInputEvents(hConsoleInput, &numEvents)) {
        return;
    }
    
    if (numEvents == 0) return;
    
    // Read the events
    INPUT_RECORD inputBuffer[128];
    if (!ReadConsoleInput(hConsoleInput, inputBuffer, 
                         min(128, (int)numEvents), &numEventsRead)) {
        return;
    }
    
    // Process each event
    for (DWORD i = 0; i < numEventsRead; i++) {
        switch (inputBuffer[i].EventType) {
            case MOUSE_EVENT: {
                MOUSE_EVENT_RECORD& mouseEvent = inputBuffer[i].Event.MouseEvent;
                
                // Update mouse position
                mouseX = mouseEvent.dwMousePosition.X;
                mouseY = mouseEvent.dwMousePosition.Y;
                
                // Update button states
                leftButtonPressed = (mouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0;
                rightButtonPressed = (mouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED) != 0;
                break;
            }
                
            case KEY_EVENT:
                // Optional: handle key events if needed
                break;
        }
    }
}
#endif // _WIN32

#ifndef _WIN32
bool MouseInput::initializeUnix() {
    // Simplified Unix implementation
    cout << "\033[?1000h"; // Enable mouse tracking
    cout.flush();
    
    mouseActive = true;
    return true;
}

void MouseInput::pollEventsUnix() {
    // Simplified Unix implementation
    // In a real implementation, you'd parse terminal escape sequences
}
#endif // !_WIN32

void MouseInput::pollEvents() {
    #ifdef _WIN32
    pollEventsWindows();
    #else
    pollEventsUnix();
    #endif
}

Vector2D MouseInput::screenToWorld(int screenX, int screenY, double radarRange) const {
    // Convert screen coordinates to world coordinates
    // Center of screen is radar position (0,0)
    
    double dx = screenX - screenCenter.x;
    double dy = screenCenter.y - screenY; // Invert Y axis
    
    // Scale to radar range (assuming 80x24 terminal)
    double worldX = dx * (radarRange * 2.0 / 80.0);
    double worldY = dy * (radarRange * 2.0 / 24.0);
    
    return Vector2D(worldX, worldY);
}

void MouseInput::worldToScreen(const Vector2D& worldPos, double radarRange, 
                              int& screenX, int& screenY) const {
    // Convert world coordinates to screen coordinates
    screenX = static_cast<int>(screenCenter.x + (worldPos.x * 80.0 / (radarRange * 2.0)));
    screenY = static_cast<int>(screenCenter.y - (worldPos.y * 24.0 / (radarRange * 2.0)));
}