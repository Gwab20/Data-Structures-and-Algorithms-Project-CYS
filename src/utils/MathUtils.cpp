#include "../../include/utils/MathUtils.hpp"
#include <string>

std::string MathUtils::bearingToCompassDirection(double bearing) {
    // Normalize bearing to 0-360
    bearing = std::fmod(bearing, 360.0);
    if (bearing < 0) bearing += 360.0;
    
    // Define compass sectors
    const char* directions[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    
    // Each sector is 45 degrees wide
    int index = static_cast<int>((bearing + 22.5) / 45.0) % 8;
    
    return directions[index];
}