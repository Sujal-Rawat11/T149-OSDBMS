#include "helpers.h"
#include <iostream>
#include <algorithm>
#include <limits>
#include <cstdlib>

using namespace std;

string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

string readLine(const string& prompt) {
    string s;
    while (true) {
        cout << prompt;
        if (!getline(cin, s)) exit(0);
        if (!s.empty()) return s;
        cout << "Cannot be empty.\n";
    }
}

float readNumber(const string& prompt, float minVal, float maxVal) {
    float x;
    while (true) {
        cout << prompt;
        if (cin >> x && x >= minVal && x <= maxVal) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return x;
        }
        if (cin.eof()) exit(0);
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input, try again.\n";
    }
}

string priorityLabel(int priority) {
    if (priority <= 2) return "HIGH";
    if (priority == 3) return "MEDIUM";
    return "LOW";
}

int labelToPriority(const string& label) {
    string l = toLower(label);
    if (l == "high" || l == "1") return 1;
    if (l == "medium" || l == "2" || l == "3") return 3;
    return 5;
}
