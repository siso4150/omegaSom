#pragma once

using namespace std;

struct config{
    int dimensionNum = 4;
    int mapRow;
    int mapCol;

    double somInitAlpha;
    double somFinAlpha;

    double somInitNbRadius;
    double somFinNbRadius;

    int somIterMax;
};