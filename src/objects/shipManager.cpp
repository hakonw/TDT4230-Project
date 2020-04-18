#include "objects/shipManager.h"
#include <algorithm>
#include <cmath>

const unsigned int minBots = 20; // if you cant handle this. Idk dude
const unsigned int maxBots = 2000; // A max, as what if there are too many

const unsigned int targetFps = 40; // Target should be 60 fps
const unsigned int allowedFpsDelta = 10; // Try to stay inside 60+-10 fps

float weightedAverageFps = 0; // Should hovered around targetFps

double totalTimeSinceLastUpdate = 0;
const double allowedUpdateRate = 5.0f; // How often is it allowed to update the number of bots

bool newlyUpdate = false;
float preUpdateFps = 0;
int deltaBots = 10;

bool dipDetected = false;
const int maxChangeInBots = 100.0f; // Not allowed to remove or add more than 20 bots
const float fallbackBotsPerFps = 1.0f;
float botsPerFps = 2.0f; // How much each new bot impacted the fps, example 2 => 2 reduction in fps per bot


int calculateAmountAdaptiveUpdateAmountBots(float &currentFps, double &time) {
    // Arbitrary weighted fps
    weightedAverageFps = (weightedAverageFps + currentFps) / 2;

    if(currentFps < (float) (targetFps - allowedFpsDelta)) {
        dipDetected = true;
    }

    totalTimeSinceLastUpdate += time;

    if (totalTimeSinceLastUpdate > allowedUpdateRate) {
        totalTimeSinceLastUpdate = 0;

        if (newlyUpdate) {
            float diffFps = preUpdateFps - weightedAverageFps; // Estimated change (dip) in fps due to spawning
            if (std::abs(diffFps) < fallbackBotsPerFps) { // Insignificant fps, default in case of instability
                printf("insig %f\n", diffFps);
                botsPerFps = fallbackBotsPerFps;
            } else {
                botsPerFps = (float) deltaBots / diffFps;
            }
            botsPerFps = std::abs(botsPerFps); // Inpact based on over or under tresh
            newlyUpdate = false;
            printf("bpfps %f\n", botsPerFps);
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
    if (std::abs(bpfps) < 0.0001f) {
        bpfps = 0.01f * (float) sgn(bpfps);
    }
    int estimatedBots = (int) truncf(bpfps * (current - (float) target));
    if (dipDetected && estimatedBots > 0) {
        dipDetected = false;
        estimatedBots = -10; // Force it to remove bots in there is a dip detected and we are around target fps
    }
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

// Adaptive update bots based on fps
void updateAmountBots(std::vector <Ship*> &bots, SceneNode* botsNode, float &currentFps, double &time) {
    // Only update bots if inside limits
    if (bots.size() > minBots && bots.size() < maxBots) {
        int deltaBots = calculateAmountAdaptiveUpdateAmountBots(currentFps, time);
        if (bots.size() + deltaBots < minBots) deltaBots = bots.size() - minBots;

        if (deltaBots > 0) { // Add bots
            // Activate old disabled bots
            for (Ship *s : bots) {
                if (deltaBots == 0) break;
                if (!s->enabled) {
                    s->enabled = true;
                    deltaBots--;
                }
            }
            // Add new bots if needed
            for (int i=0; i<deltaBots; i++) {
                Ship *ship = new Ship();
                bots.push_back(ship);
                botsNode->addChild(ship); // Add it to be rendered
                ship->position = bots.at(0)->position;
            }
            printf("adaptive bot amount: Increasing bots with %i to a total of %lu\n", deltaBots, bots.size());

        } else if (deltaBots < 0) { // Remove bots
            // Clean up old disabled bots, and then remove bots
            unsigned int disabled = 0;
            unsigned int deleted = 0;
            int tmpDeltaBots = deltaBots;
            for (unsigned int i=bots.size()-1; i>minBots; i--) {
                // TODO fix possible O(n^2), but has a low call rate, low pri
                if (!bots.at(i)->enabled) {
                    deleted++;
                    Ship* s = bots.at(i);
                    assert(bots.at(i) == botsNode->children.at(i));
                    bots.erase(bots.begin()+i);
                    botsNode->children.erase(botsNode->children.begin()+i);
                    delete s;
                } else {
                    if (tmpDeltaBots < 0) {
                        bots.at(i)->enabled = false;
                        tmpDeltaBots++; // Work towards 0
                        disabled++;
                    } else if (tmpDeltaBots == 0) {
                        break;
                    }
                }
            }
            printf("adaptive bot amount: Decreasing bots with %i (%i disabled, %i deleted) to a size of %lu active and a total %lu\n", deltaBots, disabled, deleted, bots.size()-disabled, bots.size());
        }
    }
}