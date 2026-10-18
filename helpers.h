#ifndef HELPERS_H
#define HELPERS_H

#include <string>

std::string toLower(std::string s);
std::string readLine(const std::string& prompt);
float readNumber(const std::string& prompt, float minVal, float maxVal = 1e9);
std::string priorityLabel(int priority);
int labelToPriority(const std::string& label);

#endif // HELPERS_H
