#pragma once

#include <vector>

using namespace std;

struct AcoData{
    vector<double> distPhr;
    vector<double> riskPhr;
    vector<double> heurisitc;

    double toCenter;
    double toGoal;
};