#include "../../include/radar/Radar.hpp"
#include <iostream>
#include <string>
using namespace std;

// Single, unified constructor
Radar::Radar(const Vector2D& pos, double range) 
    : position(pos), range(range), defenseGun(pos), 
      queueFront(nullptr), queueRear(nullptr), queueSize(0),
      sweepHead(nullptr), currentSweep(nullptr), sweepAngle(0.0),
      currentDetectedCount(0) {
    // Initialize detectedTargets array to null state
    for (int i = 0; i < MAX_DETECTED_TARGETS; i++) {
        detectedTargets[i] = Target(); // Assuming Target has default constructor
    }
    initializeSweep();
}

bool Radar::isInRange(const Target& target) const {
    return target.calculateHorizontalDistance(position) <= range;
}

void Radar::analyzeTarget(const Target& target, 
                         double& horizontalDist, 
                         double& displacement, 
                         double& bearing, 
                         string& direction) const {
    horizontalDist = target.calculateHorizontalDistance(position);
    displacement = target.calculateDisplacement(position);
    bearing = target.calculateBearingFrom(position);
    direction = target.getCompassDirectionFrom(position);
}

// Destructor - clean up linked lists
Radar::~Radar() {
    clearQueue();
    
    // Clean up circular list
    if (sweepHead) {
        SweepNode* current = sweepHead;
        SweepNode* toDelete;
        do {
            toDelete = current;
            current = current->next;
            delete toDelete;
        } while (current != sweepHead);
    }
}

// === QUEUE Implementation ===
void Radar::enqueueEvent(const DetectionEvent & event) {
    if(isQueueFull()) {
        // Remove oldest event if queue is full (FIFO behavior)
        dequeueEvent();
    }
    
    QueueNode* newNode = new QueueNode(event);
    if (isQueueEmpty()) {
        queueFront = queueRear = newNode;
    } else {
        queueRear->next = newNode;
        queueRear = newNode;
    }
    queueSize++;
}

DetectionEvent Radar::dequeueEvent() {
    if (isQueueEmpty()) {
        return DetectionEvent{"", false, 0.0};
    }
    
    QueueNode* temp = queueFront;
    DetectionEvent event = temp->data;
    queueFront = queueFront->next;
    
    if (queueFront == nullptr) {
        queueRear = nullptr;
    }
    
    delete temp;
    queueSize--;
    return event;
}

bool Radar::isQueueEmpty() const{
    return queueFront == nullptr;
}

bool Radar::isQueueFull() const {
    return queueSize >= MAX_QUEUE_SIZE;
}

void Radar::clearQueue() {
    while(!isQueueEmpty()) {
        dequeueEvent();
    }
}

// ===CIRCULAR LINKED LIST FOR SWEEP ===
void Radar::initializeSweep() {
    // Create circular linked list with 16 directions (0°, 22.5°, 45°, ..., 337.5°)
    const int numDirections = 16;
    double angles[numDirections];
    
    for (int i = 0; i < numDirections; i++) {
        angles[i] = i * 22.5;  // 0, 22.5, 45, 67.5, ..., 337.5
    }
    
    SweepNode* prev = nullptr;
    for (int i = 0; i < numDirections; i++) {
        SweepNode* newNode = new SweepNode(angles[i]);
        if (sweepHead == nullptr) {
            sweepHead = newNode;
            prev = sweepHead;
        } else {
            prev->next = newNode;
            prev = newNode;
        }
    }
    // Make it circular
    if (prev) {
        prev->next = sweepHead;
    }
    currentSweep = sweepHead;
    if (currentSweep) {
        sweepAngle = currentSweep->angle;
    }
}

void Radar::advanceSweep() {
    if(currentSweep) {
        currentSweep = currentSweep->next;
        if (currentSweep) {
            sweepAngle = currentSweep->angle;
        }
    }
}

double Radar::getCurrentSweepAngle() const {
    return sweepAngle;
}

// === UPDATED DETECTION SYSTEM ===
void Radar::updateDetections(Target* allTargets, int targetCount) {
    // Advance sweep each update
    advanceSweep();
    
    // Step 1: Check which detected targets left range
    for (int i = 0; i < currentDetectedCount; i++) {
        if (!isInRange(detectedTargets[i])) {
            // Create EXIT event and enqueue
            DetectionEvent exitEvent;
            exitEvent.targetId = detectedTargets[i].getId();
            exitEvent.isEntry = false;
            exitEvent.timestamp = 0.0; // You can add real timestamp if needed
            enqueueEvent(exitEvent);
            
            // Remove from detected array
            for (int j = i; j < currentDetectedCount - 1; j++) {
                detectedTargets[j] = detectedTargets[j + 1];
            }
            detectedTargets[currentDetectedCount - 1] = Target(); // Clear last element
            currentDetectedCount--;
            i--; // Adjust index after removal
        }
    }
    
    // Step 2: Check for new targets entering range
    for (int i = 0; i < targetCount; i++) {
        if (isInRange(allTargets[i])) {
            // Linear search to check if already detected - compare by ID
            bool alreadyDetected = false;
            for (int j = 0; j < currentDetectedCount; j++) {
                if (detectedTargets[j].getId() == allTargets[i].getId()) {
                    alreadyDetected = true;
                    break;
                }
            }
            
            // If new detection, add to detected targets and create ENTER event
            if (!alreadyDetected && currentDetectedCount < MAX_DETECTED_TARGETS) {
                detectedTargets[currentDetectedCount] = allTargets[i];
                currentDetectedCount++;
                
                DetectionEvent enterEvent;
                enterEvent.targetId = allTargets[i].getId();
                enterEvent.isEntry = true;
                enterEvent.timestamp = 0.0; // You can add real timestamp if needed
                enqueueEvent(enterEvent);
            }
        }
    }
} 

void Radar::printDetectionEvents() {
    cout << "Sweep Angle: " << getCurrentSweepAngle() << "° | ";
    
    if (isQueueEmpty()) {
        cout << "No detection events" << endl;
        return;
    }
    
    // Print all events in queue (FIFO order)
    int eventsPrinted = 0;
    QueueNode* current = queueFront;
    while (current != nullptr && eventsPrinted < 5) { // Limit to 5 events per print
        if (current->data.isEntry) {
            cout << "ENTER: " << current->data.targetId << " ";
        } else {
            cout << "EXIT: " << current->data.targetId << " ";
        }
        current = current->next;
        eventsPrinted++;
    }
    cout << endl;
    
    // Optional: Clear events after printing
    // clearQueue(); // Uncomment if you want to clear after printing
}