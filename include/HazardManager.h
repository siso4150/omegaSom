#pragma once

#include "omegaSom.h"
#include "config.h"
#include "colony.h"


class HazardManager{

private:
    OmegaSom* somPtr;
    Colony* colonyPtr;

    const config& cfgRef;

    mt19937_64 gen;

    inline static const int dX[] = {0, 1, 1, 1, 0, -1, -1, -1};
    inline static const int dY[] = {-1, -1, 0, 1, 1, 1, 0, -1};

public:
    HazardManager(const config&,OmegaSom*,Colony*);

    void randomRoadClose();
    void aroundClose();

};