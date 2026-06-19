#pragma once

#include "config.h"
#include "omegaSom.h"

using namespace std;

struct Coord{
    int x;
    int y;
    int d;
};



class Ant{
private:
    vector<Coord> route;
    vector<double> pVec;

    int dist;
    double risk;

    Coord cur;

    inline static const int dX[] = {0, 1, 1, 1, 0, -1, -1, -1};
    inline static const int dY[] = {-1, -1, 0, 1, 1, 1, 0, -1};

    mt19937_64 randomGen;

    vector<vector<int>> visit;
    


public:

    Ant(const config&, int seed);
    void initAnt();
    void search(const config&, const vector<Neuron>&, const vector<vector<int>>&);

    void calcProb(const config& cfgRef,const vector<Neuron>&, const vector<vector<int>>&);
    int dirSelect();
    void restart(int,int);

    int getDist() const{return dist;};
    double getRisk()const{return risk;};
    const vector<Coord>& getRoute()const{return route;}; 
    const vector<vector<int>>& getVisit()const{return visit;};
};

