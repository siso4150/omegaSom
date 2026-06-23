#include "ant.h"

Ant::Ant(const config& cfg, int seed){
    pVec.resize(8,0);
    route.clear();
    dist = 0;
    risk = 0;
    stepCnt = 0;
    visit.assign(cfg.mapRow,vector<int>(cfg.mapCol,0)); //visitのメモリ確保
    pathIdx.assign(cfg.mapRow,vector<int>(cfg.mapCol,-1));
    randomGen.seed(seed);
}

void Ant::initAnt(){
    pVec.resize(8,0);
    route.clear();
    dist = 0;
    risk = 0;

}

void Ant::restart(int x, int y){
    route.clear();
    route.reserve(100000);
    route.push_back({x,y,-1});
    dist = 0;
    risk = 0;
    stepCnt = 0;

    //visitを0埋め
    for(int i = 0; i < visit.size();i++){
        fill(visit[i].begin(),visit[i].end(),0);
    }

    for(int i = 0; i < pathIdx.size();i++){
        fill(pathIdx[i].begin(),pathIdx[i].end(),-1);
    }

    
    
}

void Ant::search(const config& cfgRef, const vector<Neuron>& mapRef, const vector<vector<int>>& tableRef){
    restart(cfgRef.startX,cfgRef.startY);

    while(true){
        cur.x = route.back().x;
        cur.y = route.back().y;

        //ゴールしたかどうかチェック
        if(cfgRef.goalX == cur.x && cfgRef.goalY == cur.y){
            route.back().d = -1;
            route.shrink_to_fit();
            break;
        }

        //最大移動回数のチェック
        stepCnt++;
        if(stepCnt > 100000){
            restart(cfgRef.startX, cfgRef.startY);
            pathIdx[cfgRef.startY][cfgRef.startX] = 0;
            continue;
        }

        //セルの選択
        calcProb(cfgRef,mapRef,tableRef);
        int dir = dirSelect();
        if(dir == -1){
            restart(cfgRef.startX,cfgRef.startY);
            pathIdx[cfgRef.startY][cfgRef.startX] = 0;
            continue;
        }

        int neuronIdx = tableRef[cur.y][cur.x];

        route.back().d = dir;
        int nextX = cur.x + dX[dir];
        int nextY = cur.y + dY[dir];

        //既に訪問している（ループなら巻き戻す)
        if(pathIdx[nextY][nextX] != -1){
            
            int loopStartIdx = pathIdx[nextY][nextX];

            while(route.size() > loopStartIdx + 1){
                Coord erase = route.back();
                pathIdx[erase.y][erase.x] = -1; // マップの記憶を消す

                int eraseNeuronIdx = tableRef[erase.y][erase.x];
                for(int n = 2; n < cfgRef.dimensionNum; n++){
                    risk -= mapRef[eraseNeuronIdx].weightVec[n];
                }
                dist--;
                route.pop_back();
            }
        }else{
            pathIdx[nextY][nextX] = route.size(); // 次のインデックスを記録
            route.push_back({nextX, nextY, -1});
            
            visit[nextY][nextX]++;
            dist++;
            
            int nextNeuronIdx = tableRef[nextY][nextX];
            
            for(int n = 2; n < cfgRef.dimensionNum; n++){
                risk += mapRef[nextNeuronIdx].weightVec[n];
            }
        }
    }
    
    
    // while(true){
    //     cur.x = route.back().x;
    //     cur.y = route.back().y;
    //     int neuronIdx = tableRef[cur.y][cur.x];

    //     if(pathIdx[cur.y][cur.x] != -1){//訪問済みかどうか
    //         int loopStartIdx = pathIdx[cur.y][cur.x];

    //         while(route.size() > loopStartIdx + 1){
    //             Coord erase = route.back();
    //             pathIdx[erase.y][erase.x] = -1;

    //             int eraseNeuronIdx = tableRef[erase.y][erase.x];
    //             for(int n = 2; n < cfgRef.dimensionNum; n++){
    //                 risk -= mapRef[eraseNeuronIdx].weightVec[n];
    //             }
    //             route.pop_back();
    //         }
    //     }else{
    //         pathIdx[cur.y][cur.x] = route.size();

    //         visit[cur.y][cur.x]++; //訪問回数を加算

    //         if(cfgRef.goalX == cur.x && cfgRef.goalY == cur.y){//ゴールに到達したか
    //             route.back().d = -1;
    //             break;
    //         }

    //         dist++;
    //         for(int n = 2; n < cfgRef.dimensionNum; n++){
    //             risk += mapRef[neuronIdx].weightVec[n];
    //         }

    //         if(dist % 100000 == 0){
            
    //             restart(cfgRef.startX,cfgRef.startY);
    //             continue;
            
    //         }

    //         calcProb(cfgRef,mapRef,tableRef);
    //         int dir = dirSelect();
    //         if(dir == -1){
    //             restart(cfgRef.startX,cfgRef.startY);
    //             pathIdx[cfgRef.startY][cfgRef.startX] = 0;
    //             continue;
    //         }

    //         route.back().d = dir;
    //         route.push_back({cur.x + dX[dir],cur.y + dY[dir],-1});
    //     }
    // }
    
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

        //既に訪れているなら、移動確率を0にする
        // if(visit[movedY][movedX] > 2){
        //     pVec[i] = 0;
        //     continue;
        // }

        if(movedNeuronIdx >= 0 && mapRef.at(movedNeuronIdx).isPossible == true){//道路であり、通行可能
            
            double distP = mapRef[movedNeuronIdx].acoData->distPhr[i] * cfgRef.acoCfg.acoPhrWeight;
            double riskP = mapRef[movedNeuronIdx].acoData->riskPhr[i] * ((double)1 - cfgRef.acoCfg.acoPhrWeight);

            pVec[i] = pow(distP+riskP,cfgRef.acoCfg.acoAlpha) * pow(mapRef[movedNeuronIdx].acoData->heurisitc[i],cfgRef.acoCfg.acoBeta);
            pVec[i] /= visit[movedY][movedX] + 1; //訪問回数に応じて選びにくくする
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
    return -1;
}