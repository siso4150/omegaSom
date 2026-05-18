#include "ant.h"

mt19937_64 Ant::randomGen(random_device{}());

Ant::Ant(){
    initAnt();
}

void Ant::initAnt(){
    pVec.resize(8,0);
    route.clear();
    dist = 0;
    risk = 0;

}

void Ant::resetAnt(){
    initAnt();
}

void Ant::restart(int x, int y){
    route.clear();
    route.push_back({x,y,-1});
    dist = 0;
    risk = 0;
}

void Ant::search(const config& cfgRef, const vector<Neuron>& mapRef, const vector<vector<int>>& tableRef){
    restart(cfgRef.startX,cfgRef.startY);
    
    while(true){
        cur.x = route.back().x;
        cur.y = route.back().y;
        int neuronIdx = tableRef[cur.y][cur.x];

        if(cfgRef.goalX == cur.x && cfgRef.goalY == cur.y){//ゴールに到達したか
            route.back().d = -1;
            cout << "->探索終了 : (dist,risk) = " << dist << "," << risk << endl;
            break;
        }

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
            cout << "探索をリスタート" << endl;
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
    double r = dist(randomGen);

    for(int i = 0; i < acm.size();i++){
        if(r < acm[i]){
            return i;
        }
    }
}