#pragma once

#include "config.h"
#include "omegaSom.h"
#include "ant.h"

#include <vector>
#include <string>
#include <fstream>

using namespace std;

class Colony{

private:
    const config* cfgPtr;
    vector<Neuron>* mapPtr;

    vector<Ant> ants;
    
    vector<vector<int>> neuronIdxTable; //一次元のsomニューロン情報を二次元のマップに移す

    int Q;

    int minDist;
    double minRisk;
    double minCost;
    vector<Coord> bestRoute;

    double maxToGoal;
    double minToGoal;

    // vector<vector<bool>> isAlreadyAdd;

    vector<vector<Coord>> bestRouteHistery;

    int runCnt;

    inline static const int dX[] = {0, 1, 1, 1, 0, -1, -1, -1};
    inline static const int dY[] = {-1, -1, 0, 1, 1, 1, 0, -1};

public:
    Colony(const config&,vector<Neuron>&);

    void run();
    void updatePhr();
    void updateSolution();

    void terminateRun();

    double normalize(double x,double xmin,double xmax){return (x-xmin) / (xmax-xmin);};

    void resultToCsv();

    const vector<vector<int>>& getNeuronIdxTable() const{
        return neuronIdxTable;
    }

    void setNeuronIdxTable(int y, int x, int val){
        neuronIdxTable[y][x] = val;
    }

    void initNeuronAcoData();

};