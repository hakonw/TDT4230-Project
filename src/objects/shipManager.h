#pragma once

#include "ship.h"

int calculateAmountAdaptiveUpdateAmountBots(float &currentFps, double &time);
int calculateBotDiff(float current, unsigned int target, float bpfps);
void updateAmountBots(std::vector <Ship*> &bots, SceneNode* botsNode, float &currentFps, double &time);


template <typename T> T clamp(const T& n, const T& lower, const T& upper);
template <typename T> int sgn(T val);

