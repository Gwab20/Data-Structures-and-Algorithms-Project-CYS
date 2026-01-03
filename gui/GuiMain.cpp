#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <random>
#include <algorithm>
#include "../include/radar/Radar.hpp"
#include "../include/radar/Target.hpp"
#include "../include/ui/ConsoleUI.hpp"
#include "RadarWidget.hpp"
#include "HudPanel.hpp"

using namespace std;

// Custom random generator for targets
class RandomGenerator {
private:
    mt19937 generator;
    uniform_real_distribution<double> realDist;
    
public:
    RandomGenerator() : 
        generator(random_device{}()),
        realDist(0.0, 1.0) {}
    
    double getRandomDouble(double min, double max) {
        return min + realDist(generator) * (max - min);
    }
    
    int getRandomInt(int min, int max) {
        uniform_int_distribution<int> dist(min, max);
        return dist(generator);
    }
    
    bool getRandomBool(double trueProbability = 0.5) {
        return realDist(generator) < trueProbability;
    }
};

// Global variables for simulation
Radar radar(Vector2D(0, 0), 1000.0);
vector<Target> targets;
RandomGenerator rng;
bool simulationRunning = true;
double currentTime = 0.0;
bool mouseAsTarget = true; // Toggle for using mouse as target
Target* mouseTarget = nullptr;
int nextTargetId = 1000;

// Mouse state
double mouseX = 0.0;
double mouseY = 0.0;
bool mouseLeftPressed = false;
bool mouseRightPressed = false;
double lastMouseUpdateTime = 0.0;

// Generate random targets
void generateRandomTargets(int count, double range) {
    for (int i = 0; i < count; i++) {
        double x = rng.getRandomDouble(-range * 1.2, range * 1.2);
        double y = rng.getRandomDouble(-range * 1.2, range * 1.2);
        double height = rng.getRandomDouble(300.0, 2000.0);
        
        // 60% chance to be enemy, 40% friendly
        TargetType type = rng.getRandomBool(0.6) ? TargetType::UNKNOWN : TargetType::FRIENDLY;
        
        string id = "TGT-" + to_string(nextTargetId++);
        targets.push_back(Target(Vector2D(x, y), height, type, id));
    }
}

// Create or update mouse target
void updateMouseTarget() {
    if (!mouseAsTarget) return;
    
    // Convert mouse coordinates to world coordinates
    double worldX = (mouseX - 400.0) * 2.0; // Assuming 800x600 window
    double worldY = (300.0 - mouseY) * 2.0; // Invert Y axis
    
    if (!mouseTarget) {
        // Create mouse target
        string id = "MOUSE-" + to_string(nextTargetId++);
        mouseTarget = new Target(Vector2D(worldX, worldY), 500.0, TargetType::UNKNOWN, id);
        
        // Find it in the targets list or add it
        bool found = false;
        for (auto& target : targets) {
            if (target.getId() == id) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            targets.push_back(*mouseTarget);
        }
    } else {
        // Update mouse target position
        mouseTarget->updatePosition(Vector2D(worldX, worldY), currentTime);
        
        // Update in targets list
        for (auto& target : targets) {
            if (target.getId() == mouseTarget->getId()) {
                target = *mouseTarget;
                break;
            }
        }
    }
}

// Update target positions
void updateTargetPositions(double timeStep) {
    currentTime += timeStep;
    
    // Update mouse target
    updateMouseTarget();
    
    // Update other targets with simple movement patterns
    static map<string, Vector2D> movementPatterns;
    
    for (auto& target : targets) {
        if (mouseTarget && target.getId() == mouseTarget->getId()) {
            continue; // Skip mouse target, already updated
        }
        
        string id = target.getId();
        
        if (movementPatterns.find(id) == movementPatterns.end()) {
            // Create unique movement pattern for each target
            movementPatterns[id] = Vector2D(
                rng.getRandomDouble(-2.0, 2.0),  // X speed
                rng.getRandomDouble(-2.0, 2.0)    // Y speed
            );
        }
        
        Vector2D newPos = target.getPosition();
        newPos.x += movementPatterns[id].x;
        newPos.y += movementPatterns[id].y;
        
        // Keep targets within reasonable bounds
        const double maxBound = 1500.0;
        if (abs(newPos.x) > maxBound) {
            newPos.x = maxBound * (newPos.x > 0 ? 0.9 : -0.9);
            movementPatterns[id].x *= -1; // Bounce off boundary
        }
        if (abs(newPos.y) > maxBound) {
            newPos.y = maxBound * (newPos.y > 0 ? 0.9 : -0.9);
            movementPatterns[id].y *= -1; // Bounce off boundary
        }
        
        target.updatePosition(newPos, currentTime);
    }
}

// Mouse callback for GLFW
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    mouseX = xpos;
    mouseY = ypos;
}

// Mouse button callback
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        mouseLeftPressed = (action == GLFW_PRESS);
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        mouseRightPressed = (action == GLFW_PRESS);
    }
}

// Main GUI application
int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(1200, 800, "Air Defense Radar Simulation", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    
    // Set up mouse callbacks
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    
    // Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    // Generate initial targets
    generateRandomTargets(5, radar.getRange());
    
    // Main loop
    double lastTime = glfwGetTime();
    
    while (!glfwWindowShouldClose(window) && simulationRunning) {
        glfwPollEvents();
        
        // Calculate delta time
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Update simulation
        updateTargetPositions(deltaTime);
        radar.updateDetections(targets.data(), static_cast<int>(targets.size()));
        
        // Fullscreen main window - NO DOCKING VERSION
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(1200, 800));
        ImGui::Begin("Air Defense Radar System", &simulationRunning, 
                    ImGuiWindowFlags_NoTitleBar | 
                    ImGuiWindowFlags_NoResize | 
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoCollapse);
        
        // Simple layout without docking
        ImGui::BeginChild("RadarDisplay", ImVec2(800, 600), true);
        renderRadarWidget(radar, targets, mouseTarget);
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        ImGui::BeginChild("TargetInfo", ImVec2(0, 600), true);
        renderTargetInfoPanel(radar, targets, mouseTarget);
        ImGui::EndChild();
        
        ImGui::BeginChild("Controls", ImVec2(600, 0), true);
        renderSystemControls(radar, targets, mouseAsTarget);
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        ImGui::BeginChild("FiringSolutions", ImVec2(0, 0), true);
        renderFiringSolutions(radar, targets);
        ImGui::EndChild();
        
        ImGui::End();
        
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
        
        // Small delay to control frame rate
        this_thread::sleep_for(chrono::milliseconds(16)); // ~60 FPS
    }
    
    // Cleanup
    if (mouseTarget) {
        delete mouseTarget;
    }
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}