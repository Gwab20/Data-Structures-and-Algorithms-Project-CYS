#include "HudPanel.hpp"
#include "../include/radar/Gun.hpp"
#include <iomanip>
#include <sstream>
#include <algorithm>

void renderTargetInfoPanel(const Radar& radar, const std::vector<Target>& targets, Target* mouseTarget) {
    ImGui::Begin("Target Information", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    // Count targets by type
    int totalInRange = 0;
    int unknownCount = 0;
    int friendlyCount = 0;
    
    for (const auto& target : targets) {
        if (radar.isInRange(target)) {
            totalInRange++;
            if (target.getType() == TargetType::UNKNOWN) {
                unknownCount++;
            } else {
                friendlyCount++;
            }
        }
    }
    
    // Summary
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Target Summary");
    ImGui::Separator();
    ImGui::Text("Total Targets: %d", static_cast<int>(targets.size()));
    ImGui::Text("In Radar Range: %d", totalInRange);
    ImGui::Text("Unknown/Hostile: %d", unknownCount);
    ImGui::Text("Friendly: %d", friendlyCount);
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Detailed target information
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Detailed Target Data");
    ImGui::Separator();
    
    if (ImGui::BeginTable("TargetTable", 7, 
                         ImGuiTableFlags_Borders | 
                         ImGuiTableFlags_RowBg | 
                         ImGuiTableFlags_ScrollY)) {
        // Table headers
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Position", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Height", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Speed", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Bearing", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableHeadersRow();
        
        // Sort targets by distance from radar
        std::vector<const Target*> sortedTargets;
        for (const auto& target : targets) {
            if (radar.isInRange(target)) {
                sortedTargets.push_back(&target);
            }
        }
        
        std::sort(sortedTargets.begin(), sortedTargets.end(),
                 [&radar](const Target* a, const Target* b) {
                     return a->calculateHorizontalDistance(radar.getPosition()) <
                            b->calculateHorizontalDistance(radar.getPosition());
                 });
        
        // Display targets
        for (const auto& targetPtr : sortedTargets) {
            const Target& target = *targetPtr;
            
            ImGui::TableNextRow();
            
            // ID
            ImGui::TableSetColumnIndex(0);
            if (mouseTarget && target.getId() == mouseTarget->getId()) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", target.getId().c_str());
            } else {
                ImGui::Text("%s", target.getId().c_str());
            }
            
            // Type
            ImGui::TableSetColumnIndex(1);
            if (target.getType() == TargetType::UNKNOWN) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "UNKNOWN");
            } else {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "FRIENDLY");
            }
            
            // Position
            ImGui::TableSetColumnIndex(2);
            Vector2D pos = target.getPosition();
            ImGui::Text("(%.0f, %.0f)", pos.x, pos.y);
            
            // Height
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.0f m", target.getHeight());
            
            // Speed
            ImGui::TableSetColumnIndex(4);
            double speed = target.getSpeed();
            if (speed > 100.0f) {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "%.0f m/s", speed);
            } else if (speed > 50.0f) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.0f m/s", speed);
            } else {
                ImGui::Text("%.0f m/s", speed);
            }
            
            // Bearing
            ImGui::TableSetColumnIndex(5);
            double bearing = target.calculateBearingFrom(radar.getPosition());
            std::string direction = target.getCompassDirectionFrom(radar.getPosition());
            ImGui::Text("%.1f° %s", bearing, direction.c_str());
            
            // Status
            ImGui::TableSetColumnIndex(6);
            double distance = target.calculateHorizontalDistance(radar.getPosition());
            if (distance < radar.getRange() * 0.3f) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "CLOSE");
            } else if (distance < radar.getRange() * 0.7f) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "MEDIUM");
            } else {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "FAR");
            }
        }
        
        ImGui::EndTable();
    }
    
    // Mouse target special section
    if (mouseTarget && radar.isInRange(*mouseTarget)) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "MOUSE TARGET (LIVE TRACKING)");
        ImGui::Separator();
        
        Vector2D pos = mouseTarget->getPosition();
        Vector2D vel = mouseTarget->getVelocity();
        Vector2D acc = mouseTarget->getAcceleration();
        double speed = mouseTarget->getSpeed();
        double distance = mouseTarget->calculateHorizontalDistance(radar.getPosition());
        double bearing = mouseTarget->calculateBearingFrom(radar.getPosition());
        std::string direction = mouseTarget->getCompassDirectionFrom(radar.getPosition());
        
        ImGui::Text("Position: (%.1f, %.1f)", pos.x, pos.y);
        ImGui::Text("Velocity: (%.1f, %.1f) m/s", vel.x, vel.y);
        ImGui::Text("Acceleration: (%.1f, %.1f) m/s²", acc.x, acc.y);
        ImGui::Text("Speed: %.1f m/s", speed);
        ImGui::Text("Distance: %.1f m", distance);
        ImGui::Text("Bearing: %.1f° %s", bearing, direction.c_str());
        ImGui::Text("Height: %.0f m", mouseTarget->getHeight());
    }
    
    ImGui::End();
}

void renderSystemControls(Radar& radar, std::vector<Target>& targets, bool& mouseAsTarget) {
    ImGui::Begin("System Controls");
    
    static int addTargetCount = 1;
    static float radarRange = static_cast<float>(radar.getRange());
    static float sweepSpeed = 45.0f;
    static bool autoSweep = true;
    
    // Radar controls
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "RADAR CONTROLS");
    ImGui::Separator();
    
    // Radar range control
    if (ImGui::SliderFloat("Radar Range", &radarRange, 500.0f, 2000.0f, "%.0f m")) {
        // Update radar range (you'll need to add a setter method to Radar class)
    }
    
    // Sweep controls
    ImGui::Checkbox("Auto Sweep", &autoSweep);
    if (autoSweep) {
        ImGui::SameLine();
        if (ImGui::Button("Advance Sweep")) {
            radar.advanceSweep();
        }
        
        if (ImGui::SliderFloat("Sweep Speed", &sweepSpeed, 10.0f, 180.0f, "%.0f°/s")) {
            // Update sweep speed (you'll need to add sweep speed control to Radar)
        }
    } else {
        ImGui::Text("Manual Sweep Control:");
        if (ImGui::Button("← Left")) {
            // Rotate sweep left
        }
        ImGui::SameLine();
        if (ImGui::Button("Right →")) {
            radar.advanceSweep();
        }
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Target controls
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "TARGET CONTROLS");
    ImGui::Separator();
    
    // Mouse target toggle
    ImGui::Checkbox("Use Mouse as Target", &mouseAsTarget);
    if (mouseAsTarget) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "ACTIVE");
        ImGui::Text("Move mouse to control target position");
        ImGui::Text("Left click: Lock target | Right click: Release");
    }
    
    // Add random targets
    ImGui::InputInt("Add Targets", &addTargetCount, 1, 5);
    addTargetCount = std::max(1, std::min(addTargetCount, 10));
    
    if (ImGui::Button("Add Random Targets")) {
        // You'll need to implement generateRandomTargets function
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Clear All Targets")) {
        targets.clear();
    }
    
    // Target type distribution
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "SYSTEM STATUS");
    ImGui::Separator();
    
    // Calculate statistics
    int inRange = 0;
    double avgSpeed = 0.0;
    double maxSpeed = 0.0;
    
    for (const auto& target : targets) {
        if (radar.isInRange(target)) {
            inRange++;
            double speed = target.getSpeed();
            avgSpeed += speed;
            maxSpeed = std::max(maxSpeed, speed);
        }
    }
    
    if (inRange > 0) {
        avgSpeed /= inRange;
    }
    
    ImGui::Text("Targets in Range: %d/%d", inRange, static_cast<int>(targets.size()));
    ImGui::Text("Average Speed: %.1f m/s", avgSpeed);
    ImGui::Text("Maximum Speed: %.1f m/s", maxSpeed);
    ImGui::Text("Radar Sweep: %.1f°", static_cast<double>(radar.getCurrentSweepAngle()));
    
    // Performance metrics
    ImGui::Spacing();
    ImGui::Text("Performance:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Good");
    
    ImGui::End();
}

void renderFiringSolutions(const Radar& radar, const std::vector<Target>& targets) {
    ImGui::Begin("Firing Solutions");
    
    // Get the defense gun
    const Gun& defenseGun = radar.getDefenseGun();
    
    // Count unknown targets in range
    std::vector<const Target*> unknownTargets;
    for (const auto& target : targets) {
        if (target.getType() == TargetType::UNKNOWN && radar.isInRange(target)) {
            unknownTargets.push_back(&target);
        }
    }
    
    if (unknownTargets.empty()) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "No hostile targets detected.");
        ImGui::Text("All clear!");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "HOSTILE TARGETS DETECTED!");
        ImGui::Text("Engagement solutions ready:");
        
        // Sort by threat level (closest first)
        std::sort(unknownTargets.begin(), unknownTargets.end(),
                 [&radar](const Target* a, const Target* b) {
                     return a->calculateHorizontalDistance(radar.getPosition()) <
                            b->calculateHorizontalDistance(radar.getPosition());
                 });
        
        // Display firing solutions for up to 3 highest priority targets
        int solutionsToShow = std::min(3, static_cast<int>(unknownTargets.size()));
        
        for (int i = 0; i < solutionsToShow; i++) {
            const Target& target = *unknownTargets[i];
            FiringSolution solution = defenseGun.calculateFiringSolution(target);
            
            ImGui::PushID(i);
            
            // Create a collapsible header for each target
            std::string header = target.getId() + " - " + solution.direction + " - " +
                                std::to_string(static_cast<int>(solution.distance)) + "m";
            
            if (ImGui::CollapsingHeader(header.c_str())) {
                // Priority indicator
                ImGui::Text("Priority: ");
                ImGui::SameLine();
                if (i == 0) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "HIGH");
                } else if (i == 1) {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "MEDIUM");
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "LOW");
                }
                
                // Solution details in a table
                if (ImGui::BeginTable("SolutionDetails", 2, 
                                     ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("Parameter");
                    ImGui::TableSetupColumn("Value");
                    
                    // Elevation
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Elevation Angle");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "%.1f°", solution.elevation);
                    
                    // Azimuth
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Azimuth Angle");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "%.1f°", solution.azimuth);
                    
                    // Distance
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Distance");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.0f m", solution.distance);
                    
                    // Direction
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Compass Direction");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", solution.direction.c_str());
                    
                    // Target speed
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Target Speed");
                    ImGui::TableSetColumnIndex(1);
                    double speed = target.getSpeed();
                    if (speed > 100.0f) {
                        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "%.0f m/s", speed);
                    } else if (speed > 50.0f) {
                        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.0f m/s", speed);
                    } else {
                        ImGui::Text("%.0f m/s", speed);
                    }
                    
                    // Lead calculation (simplified)
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Lead Adjustment");
                    ImGui::TableSetColumnIndex(1);
                    double leadAngle = speed * 0.1; // Simplified lead calculation
                    ImGui::Text("%.1f°", leadAngle);
                    
                    // Ready to fire indicator
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Status");
                    ImGui::TableSetColumnIndex(1);
                    if (solution.distance < 500.0f) {
                        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "READY TO FIRE");
                    } else if (solution.distance < 800.0f) {
                        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "IN RANGE");
                    } else {
                        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "OUT OF RANGE");
                    }
                    
                    ImGui::EndTable();
                }
                
                // Fire button
                ImGui::Spacing();
                if (ImGui::Button("ENGAGE TARGET", ImVec2(-1, 30))) {
                    // Implement engagement logic here
                    // defenseGun.fire(solution);
                }
                
                // Weapon status
                ImGui::Spacing();
                ImGui::Text("Weapon: Air Defense Gun");
                ImGui::Text("Ammo: INFINITE (Simulation)");
                ImGui::Text("Cooldown: READY");
            }
            
            ImGui::PopID();
            
            // Add separator between targets
            if (i < solutionsToShow - 1) {
                ImGui::Separator();
            }
        }
        
        // Summary
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::Text("Summary:");
        ImGui::Text("• %d hostile targets detected", static_cast<int>(unknownTargets.size()));
        ImGui::Text("• %d engagement solutions calculated", solutionsToShow);
        ImGui::Text("• Closest threat: %.0f meters", 
                   unknownTargets[0]->calculateHorizontalDistance(radar.getPosition()));
        
        // Threat level indicator
        double closestDistance = unknownTargets[0]->calculateHorizontalDistance(radar.getPosition());
        ImGui::Spacing();
        ImGui::Text("Threat Level: ");
        ImGui::SameLine();
        
        if (closestDistance < 300.0f) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "CRITICAL");
        } else if (closestDistance < 600.0f) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "HIGH");
        } else if (closestDistance < 900.0f) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "MEDIUM");
        } else {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "LOW");
        }
    }
    
    ImGui::End();
}