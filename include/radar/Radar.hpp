#ifndef RADAR_HPP
#define RADAR_HPP

#include "../utils/MathUtils.hpp"
#include "Target.hpp"
#include "Gun.hpp"
#include <vector>
#include <unordered_map>   // Phase 8: IFF hash table
#include <string>
using namespace std;

// ============================================================
//  Radar  -  detects targets, tracks kinematics, controls gun
//  Phase 8: IFF via unordered_map (O(1) friend/foe lookup)
// ============================================================
class Radar {
private:
    Vector2D position;
    double   range;
    Gun      defenseGun;

    // -- Phase 8: IFF hash table --
    // Key = target ID string, Value = known-friendly flag
    unordered_map<string, bool> friendlyRegistry;

    // -- Detection queue (linked list) --
    QueueNode* queueFront;
    QueueNode* queueRear;
    int        queueSize;
    static const int MAX_QUEUE_SIZE = 50;

    // -- Circular linked list for sweep --
    SweepNode* sweepHead;
    SweepNode* currentSweep;
    double     sweepAngle;

    // -- Detected targets array --
    static const int MAX_DETECTED_TARGETS = 20;
    Target detectedTargets[MAX_DETECTED_TARGETS];
    int    currentDetectedCount;

    // -- Engagement tracking (Phase 9 / GUI feedback) --
    string lastEngagedTargetId;
    bool   engagementActive;

public:
    Radar(const Vector2D& pos = Vector2D(0, 0), double range = 1000.0);
    ~Radar();

    // Non-copyable (Gun is non-copyable)
    Radar(const Radar&)            = delete;
    Radar& operator=(const Radar&) = delete;

    // -- Getters / Setters --
    Vector2D    getPosition() const { return position; }
    double      getRange()    const { return range; }
    void        setRange(double r)  { if (r > 0) range = r; }   // Phase 9

    const Gun&  getDefenseGun() const { return defenseGun; }

    // -- Range check --
    bool isInRange(const Target& target) const;

    // -- Analysis --
    void analyzeTarget(const Target& target,
                       double& horizontalDist,
                       double& displacement,
                       double& bearing,
                       string& direction) const;

    // -- Queue operations --
    void           enqueueEvent(const DetectionEvent& event);
    DetectionEvent dequeueEvent();
    bool           isQueueEmpty() const;
    bool           isQueueFull()  const;
    void           clearQueue();

    // -- Sweep operations --
    void   initializeSweep();
    void   advanceSweep();
    double getCurrentSweepAngle() const;

    // -- Detection / update --
    void updateDetections(Target* allTargets, int targetCount);
    void printDetectionEvents();

    // -- Phase 8: IFF --
    void registerFriendly(const string& id)   { friendlyRegistry[id] = true; }
    void unregisterFriendly(const string& id) { friendlyRegistry.erase(id); }

    // O(1) IFF check via hash map
    bool isFriendlyById(const string& id) const {
        auto it = friendlyRegistry.find(id);
        return (it != friendlyRegistry.end() && it->second);
    }

    // IFF check on a Target (type field first, then registry)
    bool identifyTarget(const Target& t) const {
        if (t.getType() == TargetType::FRIENDLY) return true;
        return isFriendlyById(t.getId());
    }

    // Get sorted unknown targets (quicksort by distance, closest-first)
    // Returns how many unknowns were found; fills outSorted up to maxOut entries
    int getSortedUnknowns(Target* allTargets, int targetCount,
                          Target** outSorted, int maxOut) const;

    // -- Phase 8: Threat classification (used by RadarWidget for colour) --
    // Returns: 0 = friendly, 1 = unknown, 2 = close hostile
    int classifyThreat(const Target& t) const {
        if (identifyTarget(t)) return 0;          // friendly
        double dist = t.calculateHorizontalDistance(position);
        if (dist < range * 0.33) return 2;        // close hostile
        return 1;                                  // unknown / far hostile
    }

    // -- Phase 9: Engagement state --
    void   setEngagement(const string& targetId) {
        lastEngagedTargetId = targetId;
        engagementActive    = true;
    }
    void   clearEngagement()  { engagementActive = false; lastEngagedTargetId = ""; }
    bool   isEngaged()        const { return engagementActive; }
    string getEngagedTarget() const { return lastEngagedTargetId; }
};

#endif  // RADAR_HPP