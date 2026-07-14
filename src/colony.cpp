#include "colony.h"
#include "acoData.h"

Colony::Colony(const config& cfg, vector<Neuron>& map): cfgPtr(&cfg),mapPtr(&map){

    minDist = 1e9;
    minRisk = 1e9;
    minCost = 1e9;

    maxToGoal = -1;
    minToGoal = 1e9;

    runCnt = 1;
    
    //table初期化
    neuronIdxTable.assign(cfgPtr->mapRow+1, std::vector<int>(cfgPtr->mapCol+1, -1));
    //二次元配列に一次元配列のインデックスを書き込む
    for(int i = 0; i < mapPtr->size();i++){

        neuronIdxTable.at(mapPtr->at(i).y).at(mapPtr->at(i).x) = i;

        mapPtr->at(i).acoData = make_unique<AcoData>();
        mapPtr->at(i).acoData->riskPhr.resize(8,1.0);
        mapPtr->at(i).acoData->distPhr.resize(8,1.0);
        mapPtr->at(i).acoData->heurisitc.resize(8,1.0);
        
    }

    random_device rd;
    unsigned int baseSeed = rd();

    //アントの初期化
    for(int i = 0; i < cfgPtr->acoCfg.antNum; i++){
        ants.push_back(Ant(*cfgPtr,baseSeed + i));
    }

    

    initNeuronAcoData();//特に,ヒューリスティック値を初期化
}

void Colony::run(){

    for(int gen = 0; gen < cfgPtr->acoCfg.acoGenNum; gen++){
        if(gen % 10 == 0){
            cout << "aco : "<< gen+1 << "世代目\n";
        }
        int cnt = 0;
        
        #pragma omp parallel for
        for(size_t i = 0; i < ants.size(); i++){
            ants[i].search(*cfgPtr,*mapPtr,neuronIdxTable);
        }

        updateSolution(gen);
        updatePhr();
    }
    outputRoute();
    runCnt++;
    terminateRun();
    
}

void Colony::updatePhr(){
    
    //フェロモンの蒸発
    double rate = ((double)1 - cfgPtr->acoCfg.evaRate);
    for(auto& neuron : *mapPtr){
        if(!neuron.acoData)continue;
        for (size_t j = 0; j < neuron.acoData->distPhr.size(); ++j) {
            int X = neuron.x + dX[j];
            int Y = neuron.y + dY[j];

            //範囲外かつ、道がある方向にのみ蒸発処理
            if(!(X >= 0 && Y >= 0 && X < cfgPtr->mapCol && Y < cfgPtr->mapRow)){
                continue;
            }
            if(neuronIdxTable[Y][X] >= 0){
                neuron.acoData->distPhr[j] = max(neuron.acoData->distPhr[j]*rate,cfgPtr->acoCfg.phrMin);
                neuron.acoData->riskPhr[j] = max(neuron.acoData->riskPhr[j]*rate,cfgPtr->acoCfg.phrMin);
            }
        }
    }

    //Q値の更新
    if (minCost > 0) {
        Q = minCost;
    }

    //フェロモンの加算
    for(const auto& ant: ants){
        //if(ant.getDist() != minDist)continue; //一番いいやつだけ加算させる
        double distAdd = Q / ant.getDist();
        double riskAdd = Q / ant.getRisk();

        vector<vector<int>> visitRef = ant.getVisit();
        
        //加算済みかどうかをチェックする
        // for(size_t i = 0; i < isAlreadyAdd.size(); i++){
        //     fill(isAlreadyAdd[i].begin(),isAlreadyAdd[i].end(),false);
        // }
        

        for(const Coord& coord : ant.getRoute()){
            if(coord.d == -1)break;

            //一度通った所は加算しない
            // if(isAlreadyAdd[coord.y][coord.x] == false){

                int neuronIdx = neuronIdxTable[coord.y][coord.x];
                int visitNum = visitRef[coord.y][coord.x];

                //最大値を設定
                mapPtr->at(neuronIdx).acoData->distPhr[coord.d] = min(cfgPtr->acoCfg.phrMax,mapPtr->at(neuronIdx).acoData->distPhr[coord.d] + (distAdd));
                mapPtr->at(neuronIdx).acoData->riskPhr[coord.d] = min(cfgPtr->acoCfg.phrMax,mapPtr->at(neuronIdx).acoData->riskPhr[coord.d] + (riskAdd));

                // isAlreadyAdd[coord.y][coord.x] = true;
            //}
        }
    }
}

void Colony::updateSolution(int time){
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
    if(time % 10 == 0){
        cout << "現在までの最適解 距離:" << minDist << " リスク:" << minRisk << " コスト:" << minCost << "\n";
    }
}

void Colony::terminateRun(){
    cout << "このマップでの最適解 距離:" << minDist << " リスク:" << minRisk << " コスト:" << minCost << "\n";
    //各種数値を出力
    result.push_back({(double)minDist,(double)minRisk,(double)minCost});
    
    bestRouteHistery.push_back(bestRoute);
    bestRoute.clear();
    minDist = 1e9;
    minRisk = 1e9;
    minCost = 1e9;


    //時刻ごとの探索が終わった後、フェロモンを蒸発させる？
    double rate = 0.0;//100%飛ばす
    for(auto& neuron : *mapPtr){
        if(!neuron.acoData)continue;
        for (size_t j = 0; j < neuron.acoData->distPhr.size(); ++j) {
            int X = neuron.x + dX[j];
            int Y = neuron.y + dY[j];

            //範囲外かつ、道がある方向にのみ蒸発処理
            if(!(X >= 0 && Y >= 0 && X < cfgPtr->mapCol && Y < cfgPtr->mapRow)){
                continue;
            }
            if(neuronIdxTable[Y][X] >= 0){
                neuron.acoData->distPhr[j] = max(neuron.acoData->distPhr[j]*rate,cfgPtr->acoCfg.phrMin);
                neuron.acoData->riskPhr[j] = max(neuron.acoData->riskPhr[j]*rate,cfgPtr->acoCfg.phrMin);
            }
        }
    }
}

// void Colony::outputRoute(){
//     ostringstream oss;
//     oss << cfgPtr->binOutputRoutePath << "route_" << setfill('0') << setw(6) << runCnt << ".csv";
//     string target = oss.str();

//     ofstream file(target);
//     if (!file.is_open()) {
//         std::cerr << "Error: ファイルを開けませんでした: " << target << "\n";;
//         return;
//     }

//     file << "x,y" << "\n";
//     for(auto& c : bestRoute){
//         file << c.x << "," << c.y << "\n";
//     }
//     file.close();
//     cout << target << "に結果を出力" << "\n";
// }

void Colony::outputRoute(){
    ostringstream oss;
    oss << cfgPtr->binOutputRoutePath << "route_" << setfill('0') << setw(6) << runCnt << ".bin";
    string target = oss.str();

    ofstream outFile(target,std::ios::binary);
    for(auto& c : bestRoute){
        saveCoord sc = {c.x,c.y};
        if(!outFile.is_open()){
            std::cerr << "error" << endl;
        }
        outFile.write(reinterpret_cast<const char*>(&sc),sizeof(saveCoord));
    }
    outFile.close();
}

// void Colony::resultParam(){
//     ostringstream oss;
//     oss << cfgPtr-binOutputParamPath << "param.csv";
//     string target = oss.str();

//     ofstream file(target);
//     if (!file.is_open()) {
//         std::cerr << "Error: ファイルを開けませんでした: " << target << "\n";;
//         return;
//     }

//     file << "minDist,minRisk,minCost" << "\n";
//     for(int i = 0; i < result.size(); i++){
//         for(auto& val : result[i]){
//             file << val << ",";
//         }
//         file << "\n";
//     }

//     file.close();
//     cout << target <<  "に距離、リスク、コストを出力" << endl;
// }

void Colony::resultParam(){//ACOが作成した経路の指標を一括出力
    ostringstream oss;
    oss << cfgPtr->binOutputParamPath << "param.bin";
    string target = oss.str();

    ofstream outFile(target,std::ios::binary);
    for(int32_t i = 0; i < result.size(); i++){
        saveParam sp = {i,static_cast<int32_t>(result[i][0]),
            static_cast<float>(result[i][1]),static_cast<float>(result[i][2])};
        if(outFile.is_open()){
            outFile.write(reinterpret_cast<const char*>(&sp),sizeof(saveParam));
        }
    }
    outFile.close();
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