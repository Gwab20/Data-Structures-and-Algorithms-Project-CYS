#ifndef RADARWIDGET_HPP
#define RADARWIDGET_HPP

#include "imgui.h"
#include "../include/radar/Radar.hpp"
#include "../include/radar/Target.hpp"
#include <vector>
#include <string>

// Render the radar display widget
void renderRadarWidget(const Radar& radar, const std::vector<Target>& targets, Target* mouseTarget = nullptr);

// Helper function to draw radar circles
void drawRadarCircle(ImDrawList* drawList, const ImVec2& center, float radius, int segments = 100);

// Helper function to draw sweep line
void drawSweepLine(ImDrawList* drawList, const ImVec2& center, float radius, float angle);

// Helper function to draw targets
void drawTargets(ImDrawList* drawList, const ImVec2& center, float scale, 
                const std::vector<Target>& targets, const Radar& radar);

// Helper function to draw compass directions
void drawCompass(ImDrawList* drawList, const ImVec2& center, float radius);

#endif // RADARWIDGET_HPP