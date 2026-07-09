#pragma once

#include "json.hpp"

using namespace std;

//ACOのコンフィグ
struct AcoConfig{
    int antNum;
    double acoGenNum;
    double acoPhrWeight;
    double acoAlpha;
    double acoBeta;
    double evaRate;
    double phrMax;
    double phrMin;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AcoConfig,
    antNum,
    acoGenNum,
    acoPhrWeight,
    acoAlpha,
    acoBeta,
    evaRate,
    phrMax,
    phrMin)

//共通＋SOMのコンフィグ
struct config{
    int dimensionNum;
    int staticDimensionNum;
    int dynamicDimensionNum;
    int mapRow;
    int mapCol;

    double somInitAlpha;
    double somFinAlpha;

    double somInitNbRadius;
    double somFinNbRadius;

    int somIterNum;
    int somIterMax;

    int somWindowSize;

    double somBeta;

    string csvDirPath;
    string binOutputPath;
    string binOutputRoutePath;
    string binOutputParamPath;

    int startX;
    int startY;
    int goalX;
    int goalY;

    int somSeed;

    AcoConfig acoCfg;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(config,
    dimensionNum,
    staticDimensionNum,
    dynamicDimensionNum,
    mapRow,
    mapCol,
    somInitAlpha,
    somFinAlpha,
    somInitNbRadius,
    somFinNbRadius,
    somIterNum,
    somIterMax,
    somWindowSize,
    somBeta,
    csvDirPath,
    binOutputPath,
    binOutputRoutePath,
    binOutputParamPath,
    startX,
    startY,
    goalX,
    goalY,
    somSeed,
    acoCfg)


