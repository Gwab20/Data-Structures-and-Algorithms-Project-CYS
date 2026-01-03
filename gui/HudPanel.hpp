#ifndef HUDPANEL_HPP
#define HUDPANEL_HPP

#include "imgui.h"
#include "../include/radar/Radar.hpp"
#include "../include/radar/Target.hpp"
#include <vector>
#include <string>

// Render target information panel
void renderTargetInfoPanel(const Radar& radar, const std::vector<Target>& targets, Target* mouseTarget = nullptr);

// Render system controls panel
void renderSystemControls(Radar& radar, std::vector<Target>& targets, bool& mouseAsTarget);

// Render firing solutions panel
void renderFiringSolutions(const Radar& radar, const std::vector<Target>& targets);

#endif // HUDPANEL_HPP