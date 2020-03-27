#include "ShipManager.h"
#include <algorithm>
#include <math.h>

int calculateAmountAdaptiveUpdateAmountBots(float &currentFps, double &time) {
    // Arbitrary weighted fps
    weightedAverageFps = (weightedAverageFps + currentFps) / 2;

    totalTime += time;

    if (totalTime > allowedUpdateRate) {
        totalTime = 0;

        if (newlyUpdate) {
            float diffFps = preUpdateFps - weightedAverageFps; // Estimated change (dip) in fps due to spawning
            if (diffFps < 2.0f) { // Insignificant fps
                botsPerFps = fallbackBotsPerFps;
            } else {
                botsPerFps = (float) deltaBots / diffFps;
            }
            newlyUpdate = false;
        }

        if (weightedAverageFps < (float) (targetFps - allowedFpsDelta) // Fps is under or over threshold
            || weightedAverageFps > (float) (targetFps + 2*allowedFpsDelta)) { // 2x max threshold to keep it sable
            deltaBots = calculateBotDiff(weightedAverageFps, targetFps, botsPerFps);
            preUpdateFps = weightedAverageFps;
            newlyUpdate = true;
            return deltaBots;
        }
    }
    return 0; // In the event that no change in bots
}

int calculateBotDiff(float current, unsigned int target, float bpfps) {
    if (abs(bpfps) < 0.0001f) {
        bpfps = 0.01f * (float) sgn(bpfps);
    }
    int estimatedBots = (int) truncf(bpfps * (current - (float) target));
    estimatedBots = clamp(estimatedBots, -maxChangeInBots, maxChangeInBots); // As bpfps is not a linear relation, clamp it
    return estimatedBots;
}

template<typename T>
T clamp(const T &n, const T &lower, const T &upper) {
    return std::max(lower, std::min(n, upper));
}

template<typename T>
int sgn(T val) {
    return (T(0) <= val) - (val < T(0));
}