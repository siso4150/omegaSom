#pragma once

#include "json.hpp"

using namespace std;

struct config{
    int dimensionNum;
    int mapRow;
    int mapCol;

    double somInitAlpha;
    double somFinAlpha;

    double somInitNbRadius;
    double somFinNbRadius;

    int somIterMax;

    int somWindowSize;

    double somBeta;

    string csvDirPath;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(config,
    dimensionNum,mapRow,mapCol,somInitAlpha,somFinAlpha,somInitNbRadius,somFinNbRadius,somIterMax,somWindowSize,somBeta,csvDirPath)

