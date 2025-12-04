#include "../include/radar/Gun.hpp"
#include "../include/radar/Target.hpp"
#include <iostream>
#include <cmath>
#include <cassert>

const double EPSILON = 0.1;

bool areClose(double a, double b) {
    return std::abs(a - b) < EPSILON;
}

void test_firing_solution() {
    Gun gun(Vector2D(0, 0));
    
    // Test 1: Target at (100, 0), height 100.
    // Horizontal dist = 100. Height = 100.
    // Angle should be atan(100/100) = 45 degrees.
    Target t1(Vector2D(100, 0), 100.0);
    FiringSolution fs1 = gun.calculateFiringSolution(t1);
    
    std::cout << "Test 1 (45 deg): Elevation = " << fs1.elevation << std::endl;
    assert(areClose(fs1.elevation, 45.0));
    assert(areClose(fs1.distance, 100.0));

    // Test 2: Target at (100, 0), height 0.
    // Angle should be 0.
    Target t2(Vector2D(100, 0), 0.0);
    FiringSolution fs2 = gun.calculateFiringSolution(t2);
    std::cout << "Test 2 (0 deg): Elevation = " << fs2.elevation << std::endl;
    assert(areClose(fs2.elevation, 0.0));

    // Test 3: Target directly above (0, 0), height 100.
    // Horizontal dist = 0.
    // Angle should be 90.
    Target t3(Vector2D(0, 0), 100.0);
    FiringSolution fs3 = gun.calculateFiringSolution(t3);
    std::cout << "Test 3 (90 deg): Elevation = " << fs3.elevation << std::endl;
    assert(areClose(fs3.elevation, 90.0));
    
    std::cout << "All tests passed!" << std::endl;
}

int main() {
    test_firing_solution();
    return 0;
}
