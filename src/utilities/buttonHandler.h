#ifndef GLOWBOX_BUTTONHANDLER_H
#define GLOWBOX_BUTTONHANDLER_H

void handleKeyboardInputs(int key, int action);
bool keyInUse(int key);
bool getAndSetKeySinglePress(int key);
void toggleBoolOnPress(bool &b, int key);

#endif //GLOWBOX_BUTTONHANDLER_H
