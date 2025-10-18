#!/bin/bash

echo "================================================"
echo "Building Anti-Aircraft Radar Project - Phase 1"
echo "================================================"

# Create build directory if it doesn't exist
mkdir -p ../build

echo "Compiling source files..."

# Compile all source files with proper include paths
g++ -std=c++11 -I../include \
    ../src/utils/MathUtils.cpp \
    ../src/radar/Target.cpp \
    ../src/radar/Gun.cpp \
    ../src/radar/Radar.cpp \
    ../src/main.cpp \
    -o ../build/radar_sim

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "✅ Build successful!"
    echo ""
    echo "Running simulation..."
    echo "================================================"
    ../build/radar_sim
else
    echo "❌ Build failed!"
    echo "Please check:"
    echo "1. Are you in the scripts/ directory?"
    echo "2. Is the folder structure correct?"
    echo "3. Do you have g++ installed?"
    exit 1
fi