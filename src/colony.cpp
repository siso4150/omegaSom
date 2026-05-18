#include "colony.h"
#include "acoData.h"

Colony::Colony(const config& cfg, vector<Neuron>& map): cfgPtr(&cfg),mapPtr(&map){

    minDist = 1e9;
    minRisk = 1e9;
    minCost = 1e9;

    maxToGoal = -1;
    minToGoal = 1e9;
    
    //table初期化
    neuronIdxTable.assign(cfgPtr->mapRow + 1, std::vector<int>(cfgPtr->mapCol + 1, -1));
    //二次元配列に一次元配列のインデックスを書き込む
    cout << cfgPtr->mapCol << "," << cfgPtr->mapRow << endl;
    for(int i = 0; i < mapPtr->size();i++){

        cout << mapPtr->at(i).y << "," << mapPtr->at(i).x << endl;
        
        neuronIdxTable[mapPtr->at(i).y][mapPtr->at(i).x] = i;

        mapPtr->at(i).acoData = make_unique<AcoData>();
        mapPtr->at(i).acoData->riskPhr.resize(8,1.0);
        mapPtr->at(i).acoData->distPhr.resize(8,1.0);
        mapPtr->at(i).acoData->heurisitc.resize(8,1.0);
        
    }

    //アントの初期化
    for(int i = 0; i < cfgPtr->acoCfg.antNum; i++){
        ants.push_back(Ant());
    }

    initNeuronAcoData();//特に,ヒューリスティック値を初期化


}

void Colony::run(){

    for(int gen = 0; gen < 20; gen++){
        int cnt = 0;
        for(auto& ant : ants){
            cnt++;
            cout << "ant" << cnt << "探索開始";
            ant.search(*cfgPtr,*mapPtr,neuronIdxTable);
        }

        updateSolution();
        updatePhr();
    }
}

void Colony::updatePhr(){
    
    //フェロモンの蒸発
    double rate = ((double)1 - cfgPtr->acoCfg.evaRate);
    for(auto& neuron : *mapPtr){
        if(!neuron.acoData)continue;
        for (size_t j = 0; j < neuron.acoData->distPhr.size(); ++j) {
            neuron.acoData->distPhr[j] *= rate;
            neuron.acoData->riskPhr[j] *= rate;
        }
    }

    //Q値の更新
    if (minCost > 0) {
        int k = static_cast<int>(std::log10(minCost)) + 1;
        Q = std::pow(10, k);
    }

    //フェロモンの加算
    for(const auto& ant: ants){
        double distAdd = Q / ant.getDist();
        double riskAdd = Q / ant.getRisk();

        for(const Coord& coord : ant.getRoute()){
            if(coord.d == -1)break;

            int neuronIdx = neuronIdxTable[coord.y][coord.x];
            mapPtr->at(neuronIdx).acoData->distPhr[coord.d] += distAdd;
            mapPtr->at(neuronIdx).acoData->riskPhr[coord.d] += riskAdd;
        }
    }
}

void Colony::updateSolution(){
    for(const auto& ant : ants){
        double cost = ant.getDist() + ant.getRisk();
        
        if(minCost > cost){
            minDist = ant.getDist();
            minRisk = ant.getRisk();
            minCost = cost;

            bestRoute.clear();
            for(const auto& coord : ant.getRoute()){
                bestRoute.push_back(coord);
            }
        }
    }
    cout << "現在までの最適解 距離:" << minDist << " リスク:" << minRisk << " コスト:" << minCost << endl;
}

void Colony::initNeuronAcoData(){//とりあえずゴールまでの距離だけでヒューリスティック値を付ける
    for(int i = 0; i < mapPtr->size(); i++){
        double tmp = (mapPtr->at(i).x - cfgPtr->goalX)*(mapPtr->at(i).x - cfgPtr->goalX) + (mapPtr->at(i).y - cfgPtr->goalY) * (mapPtr->at(i).y - cfgPtr->goalY);
        mapPtr->at(i).acoData->toGoal = tmp;
        maxToGoal = max(tmp,maxToGoal);
        minToGoal = min(tmp,minToGoal);
    }

    for(int i = 0; i < mapPtr->size(); i++){
        double x = mapPtr->at(i).acoData->toGoal;
        mapPtr->at(i).acoData->toGoal = normalize(x,minToGoal,maxToGoal);
    }

    for(int i = 0; i < mapPtr->size(); i++){
        for(int d = 0; d < 8; d++){
            int movedX = mapPtr->at(i).x + dX[d];
            int movedY = mapPtr->at(i).y + dY[d];

            if(!(movedX >= 0 && movedY >= 0 && movedX < cfgPtr->mapCol && movedY < cfgPtr->mapRow)){
                continue;
            }
            int movedNeuronIdx = neuronIdxTable[movedY][movedX];

            if(movedX >= 0 && movedY >= 0 && movedX < cfgPtr->mapCol && movedY < cfgPtr->mapRow && movedNeuronIdx >= 0){
                mapPtr->at(i).acoData->heurisitc[d] = ((double)1 / (mapPtr->at(movedNeuronIdx).acoData->toGoal + 0.01));
            }
        }
    }
}