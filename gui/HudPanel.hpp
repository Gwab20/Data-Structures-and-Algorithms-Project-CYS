#ifndef HUDPANEL_HPP
#define HUDPANEL_HPP

#include "imgui.h"
#include "../include/radar/Radar.hpp"
#include "../include/radar/Target.hpp"
#include <vector>
#include <string>

// Left panel tabs
void renderTrackPanel   (const Radar& radar, const std::vector<Target>& targets,
                         Target* mouseTarget, std::string& selectedId);
void renderControlsPanel(Radar& radar, std::vector<Target>& targets,
                         bool& mouseAsTarget);

// Bottom-right tabs
void renderFireControl  (Radar& radar, const std::vector<Target>& targets);
void renderThreatPanel  (const Radar& radar, const std::vector<Target>& targets,
                         const char logBuf[][80], int logN);

#endif