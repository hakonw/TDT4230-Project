#pragma once

#include "ship.h"

const static unsigned int minBots = 20; // if you cant handle this. Idk dude
const static unsigned int maxBots = 2000; // A max, as what if there are too many

const static unsigned int targetFps = 60; // Target should be 60 fps
const static unsigned int allowedFpsDelta = 10; // Try to stay inside 60+-10 fps

static float weightedAverageFps = 0; // Should hovered around targetFps

static double totalTime = 0;
const static double allowedUpdateRate = 5.0f; // How often is it allowed to update the number of bots

static bool newlyUpdate = false;
static float preUpdateFps = 0;
static int deltaBots = 10;

const static int maxChangeInBots = 50.0f; // Not allowed to remove or add more than 20 bots
const static float fallbackBotsPerFps = 1.0f;
static float botsPerFps = 2.0f; // How much each new bot impacted the fps, example 2 => 2 reduction in fps per bot

int calculateAmountAdaptiveUpdateAmountBots(float &currentFps, double &time);
int calculateBotDiff(float current, unsigned int target, float bpfps);


template <typename T> T clamp(const T& n, const T& lower, const T& upper);
template <typename T> int sgn(T val);

