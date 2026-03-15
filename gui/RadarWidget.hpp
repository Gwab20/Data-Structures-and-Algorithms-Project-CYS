#ifndef RADARWIDGET_HPP
#define RADARWIDGET_HPP

#include "imgui.h"
#include "../include/radar/Radar.hpp"
#include "../include/radar/Target.hpp"
#include <vector>
#include <string>

// Renders the radar scope inside an existing child window.
// selectedId: if non-empty, draws a white selection ring around that target.
void renderRadarWidget(const Radar& radar,
                       const std::vector<Target>& targets,
                       Target* mouseTarget = nullptr,
                       const std::string& selectedId = "");

#endif