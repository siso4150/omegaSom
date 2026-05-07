#pragma once

#include <vector>

using namespace std;

struct MapCell{
    int x;
    int y;
    vector<double> vec;
    bool isRoad;
};

class DisasterMap{

private:
    vector<MapCell> disasterMap;

public:
    void loadFromCsv();
    const vector<MapCell>& getDisasterMap() const {return disasterMap;};

};