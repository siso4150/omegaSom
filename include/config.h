#pragma once

#include "json.hpp"

using namespace std;

//ACOのコンフィグ
struct AcoConfig{
    int antNum;
    double acoPhrWeight;
    double acoAlpha;
    double acoBeta;
    double evaRate;
    double phrMax;
    double phrMin;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AcoConfig,antNum,acoPhrWeight,acoAlpha,acoBeta,evaRate,phrMax,phrMin)

//共通＋SOMのコンフィグ
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
    string csvOutputPath;

    int startX;
    int startY;
    int goalX;
    int goalY;

    AcoConfig acoCfg;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(config,dimensionNum,mapRow,mapCol,somInitAlpha,somFinAlpha,somInitNbRadius,somFinNbRadius,
    somIterMax,somWindowSize,somBeta,csvDirPath,csvOutputPath,startX,startY,goalX,goalY,acoCfg)


