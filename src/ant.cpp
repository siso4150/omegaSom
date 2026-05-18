#include "ant.h"

Ant::Ant(){
    initAnt();
}

void Ant::initAnt(){
    pVec.resize(8,0);
    route.clear();

}

void Ant::resetAnt(){
    initAnt();
}

void Ant::restart(int x, int y){
    route.clear();
    route.push_back({x,y,-1});
}

void Ant::search(const config& cfgRef, const vector<Neuron>& mapRef, const vector<vector<int>>& tableRef){
    restart(cfgRef.startX,cfgRef.startY);
    
    while(true){
        cur.x = route.back().x;
        cur.y = route.back().y;
        int neuronIdx = tableRef[cur.x][cur.y];

        dist++;
        for(int n = 2; n < cfgRef.dimensionNum; n++){
            risk+= mapRef[neuronIdx].weightVec[n];
        }

        if(dist % 10000 == 0){
            //cout << "探索" << dist << "ステップ目" << endl;
            if(dist ==  100000){
                cout << "探索をリスタート" << endl;
                restart(cfgRef.startX,cfgRef.startY);
                continue;
            }
        }

        calcProb(cfgRef,mapRef,tableRef);
        int dir = dirSelect();
        if(dir == -1){
            restart(cfgRef.startX,cfgRef.startY);
            continue;
        }

        route.back().d = dir;
        route.push_back({cur.x + dX[dir],cur.y + dY[dir],-1});
    
    }
}

void Ant::calcProb(const config& cfgRef,const vector<Neuron>& mapRef, const vector<vector<int>>& tableRef){
    pVec.clear();
    pVec.resize(8,0);

    for(int i = 0; i < 8; i++){
        int movedX = cur.x + dX[i];
        int movedY = cur.y + dY[i];
        if(!(movedX >= 0 && movedY >= 0 && movedX < cfgRef.mapCol && movedY < cfgRef.mapRow)){
           continue;
        }
        int movedNeuronIdx = tableRef[movedY][movedX];

        if(movedNeuronIdx >= 0){
            
            double distP = mapRef[movedNeuronIdx].acoData->distPhr[i] * cfgRef.acoCfg.acoPhrWeight;
            double riskP = mapRef[movedNeuronIdx].acoData->riskPhr[i] * ((double)1 - cfgRef.acoCfg.acoPhrWeight);

            cout << mapRef.size() << endl;
            cout << pVec.size() << endl;

            pVec[i] = pow(distP+riskP,cfgRef.acoCfg.acoAlpha) * pow(mapRef[movedNeuronIdx].acoData->heurisitc[i],cfgRef.acoCfg.acoBeta);
        }
    }
}

int Ant::dirSelect(){
    double sum = 0;
    for(auto val : pVec) sum += val;
    if(sum == 0) return -1;//すべての移動セルが移動不可のため戻して最初からやり直し

    for(int i = 0; i < pVec.size();i++) pVec[i] /= sum;
    vector<double> acm(8,0);
    acm[0] = pVec[0];
    for(int i = 1; i < pVec.size();i++) acm[i] = acm[i-1] + pVec[i];

    uniform_real_distribution<double> dist(0,1);
    double r = dist(gen);

    for(int i = 0; i < acm.size();i++){
        if(r < acm[i]){
            return i;
        }
    }
}