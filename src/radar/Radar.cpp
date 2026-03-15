#include "../../include/radar/Radar.hpp"
#include <iostream>
#include <algorithm>
#include <string>
using namespace std;

// ============================================================
//  Radar implementation
//  Phases 1-9 complete: detection, queue, sweep, IFF, engagement
// ============================================================

Radar::Radar(const Vector2D& pos, double range)
    : position(pos), range(range), defenseGun(pos),
      queueFront(nullptr), queueRear(nullptr), queueSize(0),
      sweepHead(nullptr), currentSweep(nullptr), sweepAngle(0.0),
      currentDetectedCount(0),
      lastEngagedTargetId(""), engagementActive(false)
{
    for (int i = 0; i < MAX_DETECTED_TARGETS; i++) {
        detectedTargets[i] = Target();
    }
    initializeSweep();
}

Radar::~Radar() {
    clearQueue();
    if (sweepHead) {
        SweepNode* cur = sweepHead;
        SweepNode* del;
        do {
            del = cur;
            cur = cur->next;
            delete del;
        } while (cur != sweepHead);
    }
}

// ── Range check ──────────────────────────────────────────────
bool Radar::isInRange(const Target& target) const {
    return target.calculateHorizontalDistance(position) <= range;
}

void Radar::analyzeTarget(const Target& target,
                          double& horizontalDist,
                          double& displacement,
                          double& bearing,
                          string& direction) const
{
    horizontalDist = target.calculateHorizontalDistance(position);
    displacement   = target.calculateDisplacement(position);
    bearing        = target.calculateBearingFrom(position);
    direction      = target.getCompassDirectionFrom(position);
}

// ── Queue ─────────────────────────────────────────────────────
void Radar::enqueueEvent(const DetectionEvent& event) {
    if (isQueueFull()) dequeueEvent();     // drop oldest if full

    QueueNode* newNode = new QueueNode(event);
    if (isQueueEmpty()) {
        queueFront = queueRear = newNode;
    } else {
        queueRear->next = newNode;
        queueRear       = newNode;
    }
    queueSize++;
}

DetectionEvent Radar::dequeueEvent() {
    if (isQueueEmpty()) return DetectionEvent{"", false, 0.0};

    QueueNode*     temp  = queueFront;
    DetectionEvent event = temp->data;
    queueFront           = queueFront->next;
    if (queueFront == nullptr) queueRear = nullptr;
    delete temp;
    queueSize--;
    return event;
}

bool Radar::isQueueEmpty() const { return queueFront == nullptr; }
bool Radar::isQueueFull()  const { return queueSize >= MAX_QUEUE_SIZE; }

void Radar::clearQueue() {
    while (!isQueueEmpty()) dequeueEvent();
}

// ── Circular linked list sweep ────────────────────────────────
void Radar::initializeSweep() {
    const int numDirections = 16;   // 0°, 22.5°, …, 337.5°
    SweepNode* prev = nullptr;

    for (int i = 0; i < numDirections; i++) {
        SweepNode* node = new SweepNode(i * 22.5);
        if (sweepHead == nullptr) {
            sweepHead = node;
            prev      = sweepHead;
        } else {
            prev->next = node;
            prev       = node;
        }
    }
    if (prev) prev->next = sweepHead;   // make circular
    currentSweep = sweepHead;
    if (currentSweep) sweepAngle = currentSweep->angle;
}

void Radar::advanceSweep() {
    if (currentSweep) {
        currentSweep = currentSweep->next;
        if (currentSweep) sweepAngle = currentSweep->angle;
    }
}

double Radar::getCurrentSweepAngle() const { return sweepAngle; }

// ── Detection system ──────────────────────────────────────────
void Radar::updateDetections(Target* allTargets, int targetCount) {
    advanceSweep();

    // Step 1: remove targets that left range
    for (int i = 0; i < currentDetectedCount; i++) {
        if (!isInRange(detectedTargets[i])) {
            DetectionEvent exitEv;
            exitEv.targetId  = detectedTargets[i].getId();
            exitEv.isEntry   = false;
            exitEv.timestamp = 0.0;
            enqueueEvent(exitEv);

            for (int j = i; j < currentDetectedCount - 1; j++)
                detectedTargets[j] = detectedTargets[j + 1];
            detectedTargets[currentDetectedCount - 1] = Target();
            currentDetectedCount--;
            i--;
        }
    }

    // Step 2: add newly-in-range targets
    for (int i = 0; i < targetCount; i++) {
        if (!isInRange(allTargets[i])) continue;

        // Linear search – is it already tracked?
        bool found = false;
        for (int j = 0; j < currentDetectedCount; j++) {
            if (detectedTargets[j].getId() == allTargets[i].getId()) {
                detectedTargets[j] = allTargets[i]; // refresh data
                found = true;
                break;
            }
        }

        if (!found && currentDetectedCount < MAX_DETECTED_TARGETS) {
            detectedTargets[currentDetectedCount++] = allTargets[i];

            // -- Phase 8: auto-register type from target object --
            if (allTargets[i].getType() == TargetType::FRIENDLY) {
                registerFriendly(allTargets[i].getId());
            }

            DetectionEvent enterEv;
            enterEv.targetId  = allTargets[i].getId();
            enterEv.isEntry   = true;
            enterEv.timestamp = 0.0;
            enqueueEvent(enterEv);
        }
    }
}

void Radar::printDetectionEvents() {
    cout << "Sweep: " << getCurrentSweepAngle() << "deg | ";
    if (isQueueEmpty()) { cout << "No events" << endl; return; }

    int printed = 0;
    QueueNode* cur = queueFront;
    while (cur && printed < 5) {
        cout << (cur->data.isEntry ? "ENTER:" : "EXIT:") << cur->data.targetId << " ";
        cur = cur->next;
        printed++;
    }
    cout << endl;
}

// ── Phase 8: getSortedUnknowns ────────────────────────────────
// Collects in-range unknown targets, then quicksorts by distance.
// Returns count of unknowns found (up to maxOut).
static void quicksortByDist(Target** arr, int lo, int hi, const Vector2D& origin) {
    if (lo >= hi) return;
    double pivot = arr[(lo + hi) / 2]->calculateHorizontalDistance(origin);
    int i = lo, j = hi;
    while (i <= j) {
        while (arr[i]->calculateHorizontalDistance(origin) < pivot) i++;
        while (arr[j]->calculateHorizontalDistance(origin) > pivot) j--;
        if (i <= j) { swap(arr[i], arr[j]); i++; j--; }
    }
    quicksortByDist(arr, lo, j, origin);
    quicksortByDist(arr, i, hi, origin);
}

int Radar::getSortedUnknowns(Target* allTargets, int targetCount,
                              Target** outSorted, int maxOut) const
{
    int count = 0;
    for (int i = 0; i < targetCount && count < maxOut; i++) {
        if (isInRange(allTargets[i]) && !identifyTarget(allTargets[i])) {
            outSorted[count++] = &allTargets[i];
        }
    }
    if (count > 1)
        quicksortByDist(outSorted, 0, count - 1, position);
    return count;
}