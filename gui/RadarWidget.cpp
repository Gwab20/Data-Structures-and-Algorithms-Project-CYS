#include "RadarWidget.hpp"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void renderRadarWidget(const Radar& radar, const std::vector<Target>& targets, Target* mouseTarget) {
    ImGui::Begin("Radar Display");
    
    // Get available space
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    canvasSize.y -= 40; // Leave space for status text
    
    // Calculate center and radius
    ImVec2 center = ImVec2(
        ImGui::GetCursorScreenPos().x + canvasSize.x * 0.5f,
        ImGui::GetCursorScreenPos().y + canvasSize.y * 0.5f
    );
    float radius = std::min(canvasSize.x, canvasSize.y) * 0.45f;
    
    // Create draw list
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // Draw radar background
    drawList->AddCircleFilled(center, radius, IM_COL32(10, 20, 40, 255));
    
    // Draw concentric circles (range rings)
    drawRadarCircle(drawList, center, radius);
    
    // Draw compass directions
    drawCompass(drawList, center, radius);
    
    // Draw sweep line
    float sweepAngle = static_cast<float>(radar.getCurrentSweepAngle());
    drawSweepLine(drawList, center, radius, sweepAngle);
    
    // Calculate scale factor
    float scale = radius / static_cast<float>(radar.getRange());
    
    // Draw targets
    drawTargets(drawList, center, scale, targets, radar);
    
    // Highlight mouse target if it exists
    if (mouseTarget && radar.isInRange(*mouseTarget)) {
        Vector2D pos = mouseTarget->getPosition();
        ImVec2 targetPos = ImVec2(
            center.x + pos.x * scale,
            center.y - pos.y * scale // Invert Y for screen coordinates
        );
        
        drawList->AddCircleFilled(targetPos, 8.0f, IM_COL32(255, 0, 0, 255)); // Red for mouse target
        drawList->AddCircle(targetPos, 10.0f, IM_COL32(255, 255, 255, 255), 0, 2.0f);
        
        // Draw label
        std::string label = mouseTarget->getId() + " (MOUSE)";
        ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        drawList->AddText(ImVec2(targetPos.x - textSize.x * 0.5f, targetPos.y + 15.0f), 
                         IM_COL32(255, 255, 255, 255), label.c_str());
    }
    
    // Draw center crosshair
    drawList->AddLine(ImVec2(center.x - 10, center.y), ImVec2(center.x + 10, center.y), 
                     IM_COL32(255, 255, 255, 200), 2.0f);
    drawList->AddLine(ImVec2(center.x, center.y - 10), ImVec2(center.x, center.y + 10), 
                     IM_COL32(255, 255, 255, 200), 2.0f);
    
    // Draw radar position
    drawList->AddCircleFilled(center, 5.0f, IM_COL32(0, 255, 0, 255));
    
    // Status text
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + canvasSize.y);
    ImGui::Separator();
    ImGui::Text("Sweep Angle: %.1f° | Range: %.0fm | Targets in range: %d/%d", 
                sweepAngle, radar.getRange(), 
                [&]() {
                    int count = 0;
                    for (const auto& target : targets) {
                        if (radar.isInRange(target)) count++;
                    }
                    return count;
                }(),
                static_cast<int>(targets.size()));
    
    if (mouseTarget) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), " | Mouse Tracking: ACTIVE");
    }
    
    ImGui::End();
}

void drawRadarCircle(ImDrawList* drawList, const ImVec2& center, float radius, int segments) {
    // Draw three concentric circles at 33%, 66%, 100% range
    for (int i = 1; i <= 3; i++) {
        float circleRadius = radius * i / 3.0f;
        ImU32 color;
        float thickness;
        
        if (i == 1) {
            color = IM_COL32(0, 150, 0, 100);      // Inner circle - green
            thickness = 1.0f;
        } else if (i == 2) {
            color = IM_COL32(255, 255, 0, 100);    // Middle circle - yellow
            thickness = 1.5f;
        } else {
            color = IM_COL32(255, 0, 0, 100);      // Outer circle - red
            thickness = 2.0f;
        }
        
        drawList->AddCircle(center, circleRadius, color, segments, thickness);
    }
}

void drawSweepLine(ImDrawList* drawList, const ImVec2& center, float radius, float angle) {
    // Convert angle to radians
    float rad = angle * static_cast<float>(M_PI) / 180.0f;
    
    // Calculate end point
    ImVec2 endPoint = ImVec2(
        center.x + radius * cosf(rad),
        center.y - radius * sinf(rad) // Negative because screen Y increases downward
    );
    
    // Draw sweep line with gradient effect
    ImU32 colors[] = {
        IM_COL32(0, 255, 0, 255),    // Bright green at center
        IM_COL32(0, 255, 0, 200),    // 
        IM_COL32(0, 255, 0, 150),    // 
        IM_COL32(0, 255, 0, 100),    // 
        IM_COL32(0, 255, 0, 50)      // Faded at edge
    };
    
    for (int i = 0; i < 5; i++) {
        float lineRadius = radius * (1.0f - i * 0.05f);
        ImVec2 lineEnd = ImVec2(
            center.x + lineRadius * cosf(rad),
            center.y - lineRadius * sinf(rad)
        );
        drawList->AddLine(center, lineEnd, colors[i], 2.0f - i * 0.3f);
    }
    
    // Draw sweep arc (trail effect)
    for (int i = 0; i < 10; i++) {
        float trailAngle = angle - i * 2.0f; // Trail behind sweep
        if (trailAngle < 0) trailAngle += 360.0f;
        
        float trailRad = trailAngle * static_cast<float>(M_PI) / 180.0f;
        float trailRadius = radius * (1.0f - i * 0.02f);
        
        ImVec2 trailPoint = ImVec2(
            center.x + trailRadius * cosf(trailRad),
            center.y - trailRadius * sinf(trailRad)
        );
        
        ImU32 trailColor = IM_COL32(0, 100, 0, 50 - i * 5); // Fading trail
        drawList->AddCircleFilled(trailPoint, 2.0f, trailColor);
    }
}

void drawTargets(ImDrawList* drawList, const ImVec2& center, float scale, 
                const std::vector<Target>& targets, const Radar& radar) {
    for (const Target& target : targets) {
        if (!radar.isInRange(target)) {
            continue;
        }
        
        Vector2D pos = target.getPosition();
        ImVec2 targetPos = ImVec2(
            center.x + pos.x * scale,
            center.y - pos.y * scale // Invert Y for screen coordinates
        );
        
        // Choose color based on target type
        ImU32 color;
        float radius;
        
        switch (target.getType()) {
            case TargetType::FRIENDLY:
                color = IM_COL32(0, 255, 255, 255); // Cyan for friendly
                radius = 6.0f;
                break;
            case TargetType::UNKNOWN:
            default:
                color = IM_COL32(255, 255, 0, 255); // Yellow for unknown
                radius = 8.0f;
                break;
        }
        
        // Draw target
        drawList->AddCircleFilled(targetPos, radius, color);
        
        // Draw outline
        drawList->AddCircle(targetPos, radius + 2.0f, IM_COL32(255, 255, 255, 200), 0, 1.5f);
        
        // Draw speed vector if target is moving
        Vector2D velocity = target.getVelocity();
        float speed = target.getSpeed();
        
        if (speed > 0.1f) {
            // Scale velocity for visualization
            float vectorScale = 10.0f;
            ImVec2 velocityEnd = ImVec2(
                targetPos.x + velocity.x * vectorScale,
                targetPos.y - velocity.y * vectorScale // Invert Y
            );
            
            drawList->AddLine(targetPos, velocityEnd, IM_COL32(255, 100, 100, 200), 2.0f);
            drawList->AddCircleFilled(velocityEnd, 3.0f, IM_COL32(255, 50, 50, 200));
            
            // Draw speed text
            std::string speedText = std::to_string(static_cast<int>(speed)) + " m/s";
            ImVec2 textSize = ImGui::CalcTextSize(speedText.c_str());
            drawList->AddText(ImVec2(targetPos.x - textSize.x * 0.5f, targetPos.y - 20.0f), 
                             IM_COL32(255, 200, 200, 255), speedText.c_str());
        }
        
        // Draw target ID
        std::string idText = target.getId();
        ImVec2 textSize = ImGui::CalcTextSize(idText.c_str());
        drawList->AddText(ImVec2(targetPos.x - textSize.x * 0.5f, targetPos.y + 10.0f), 
                         IM_COL32(255, 255, 255, 255), idText.c_str());
    }
}

void drawCompass(ImDrawList* drawList, const ImVec2& center, float radius) {
    // Draw N, E, S, W markers
    const char* directions[] = {"N", "E", "S", "W"};
    float angles[] = {90.0f, 0.0f, 270.0f, 180.0f}; // In degrees
    
    for (int i = 0; i < 4; i++) {
        float rad = angles[i] * static_cast<float>(M_PI) / 180.0f;
        ImVec2 dirPos = ImVec2(
            center.x + (radius + 20.0f) * cosf(rad),
            center.y - (radius + 20.0f) * sinf(rad) // Negative because screen Y increases downward
        );
        
        ImVec2 textSize = ImGui::CalcTextSize(directions[i]);
        drawList->AddText(ImVec2(dirPos.x - textSize.x * 0.5f, dirPos.y - textSize.y * 0.5f), 
                         IM_COL32(255, 255, 255, 255), directions[i]);
        
        // Draw small tick mark
        ImVec2 tickStart = ImVec2(
            center.x + radius * cosf(rad),
            center.y - radius * sinf(rad)
        );
        ImVec2 tickEnd = ImVec2(
            center.x + (radius + 10.0f) * cosf(rad),
            center.y - (radius + 10.0f) * sinf(rad)
        );
        drawList->AddLine(tickStart, tickEnd, IM_COL32(255, 255, 255, 200), 2.0f);
    }
    
    // Draw intermediate directions (NE, SE, SW, NW)
    const char* subDirections[] = {"NE", "SE", "SW", "NW"};
    float subAngles[] = {45.0f, 315.0f, 225.0f, 135.0f};
    
    for (int i = 0; i < 4; i++) {
        float rad = subAngles[i] * static_cast<float>(M_PI) / 180.0f;
        ImVec2 dirPos = ImVec2(
            center.x + (radius + 15.0f) * cosf(rad),
            center.y - (radius + 15.0f) * sinf(rad)
        );
        
        ImVec2 textSize = ImGui::CalcTextSize(subDirections[i]);
        drawList->AddText(ImVec2(dirPos.x - textSize.x * 0.5f, dirPos.y - textSize.y * 0.5f), 
                         IM_COL32(200, 200, 200, 200), subDirections[i]);
    }
}