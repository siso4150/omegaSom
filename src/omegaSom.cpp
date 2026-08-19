#include "omegaSom.h"

//#define DEBUG

using namespace std;

OmegaSom::OmegaSom(const config& cfg,const vector<MapCell>& dMap): cfg(cfg),disasterMap(dMap){
    gen.seed(cfg.somSeed);
    uniform_real_distribution<double> rdist(0,1);

    semiBatchRandEngine.seed(cfg.semiBatchSeed);
    
    //メモリ領域確保
    somMap.reserve(disasterMap.size());

    for(int i = 0; i < disasterMap.size();i++){
        if(disasterMap[i].isRoad == false){
            continue;
        }else{
            Neuron neuron;
            neuron.x = disasterMap[i].x;
            neuron.y = disasterMap[i].y;

            neuron.weightVec.assign(cfg.dimensionNum,0.0);

            for(int n = 0; n < cfg.dimensionNum; n++){
                neuron.weightVec[n] = (rdist(gen));
            }

            neuron.weightVec[0] = neuron.x;
            neuron.weightVec[1] = neuron.y;

            neuron.isPossible = true;

            neuron.inflDenominator = 0.0;
            neuron.inflNumerator.assign(cfg.dimensionNum,0.0);

            somMap.push_back(move(neuron));
        }
    }

    //メモリ領域削減
    somMap.shrink_to_fit();

    double initialWeight = 1.0 / cfg.dimensionNum;
    omega.assign(cfg.dimensionNum,initialWeight);
    density.assign(cfg.dimensionNum,0);
    runningSum.assign(cfg.somWindowSize,0);

    BMUIdxes.assign(disasterMap.size(),0);

    for(int n = 0; n < cfg.dimensionNum; n++){
        omegaHistery.push_back(vector<double>(cfg.somWindowSize,0.0));
        
        for(int i = 0; i < cfg.somWindowSize; i++){
            omegaHistery[n][i] = initialWeight;
        }
        runningSum[n] = initialWeight * cfg.somWindowSize;
        omega[n] = initialWeight;
    }

    preCalcOmega.resize(cfg.dimensionNum);

    cout << "0世代目" << "\n";
    for(auto val : omega) cout << val << " ";
    cout << "\n";

    alpha = cfg.somInitAlpha;
    nbRadius = cfg.somInitNbRadius;
    tau = 20;
    resetLocalIter();
}

void OmegaSom::onlineLearn(int t){//オンライン学習
    //入力データをランダムに一つ選ぶ
    uniform_int_distribution<int> dist(0,disasterMap.size()-1);
    int inputIdx = dist(gen);

    bool flag = false;
    for(auto val : disasterMap[inputIdx].vec){
        
        if(val > 1.0){
            flag = true;
        }
        
    }
    if(flag == true){
            std::cerr << "入力データが正しく正規化されていません" << "\n";
            abort();
        }

    int BMUIdx = findBMU(inputIdx); //BMU探索

    onlineAdapt(BMUIdx,inputIdx); //プロトタイプベクトル（参照ベクトル）更新
    updateOmega(BMUIdx,inputIdx,t); //次元重み更新
    updateAlphaNb(); //学習率、近傍半径の更新
}

void OmegaSom::batchLearn(int t){

    #ifdef DEBUG
    double startTime = omp_get_wtime();
    #endif 

    //一旦全ての入力データのBMUを計算する
    #pragma omp parallel for
    for(int k = 0; k < static_cast<int>(disasterMap.size()); k++){
        BMUIdxes[k] = findBMU(k);
    }

    #ifdef DEBUG
    cout << "BMU計算完了\n";
    #endif 

    //各ニューロン
    #pragma omp parallel for
    for(int i = 0; i < static_cast<int>(somMap.size()); i++){
        
        //初期化
        somMap[i].inflDenominator = 0.0;
        for(int n = 0; n < cfg.dimensionNum; n++){
            somMap[i].inflNumerator[n] = 0.0;
        }

        //各入力データで影響度を計算する
        for(int j = 0; j < static_cast<int>(disasterMap.size());j++){

            double nb = neighborhoodFunction(BMUIdxes[j],i);
            if(nb < 0.046) continue;
            somMap[i].inflDenominator += nb;

            for(int n = 0; n < cfg.dimensionNum; n++){
                somMap[i].inflNumerator[n] += nb*disasterMap[j].vec[n];
            }
        }
    }

    #ifdef DEBUG
    double endTime = omp_get_wtime();
    double time = endTime - startTime;
    cout << "バッチ学習時間:" << time << "\n";
    #endif

    #ifdef DEBUG
    cout << "影響量計算完了\n";
    #endif 

    batchAdapt(t);
    
    #ifdef DEBUG
    cout << "影響量の更新完了\n";
    #endif 

    #ifdef DEBUG
    startTime = omp_get_wtime();
    #endif

    batchUpdateOmega(t);

    #ifdef DEBUG
    endTime = omp_get_wtime();
    time = endTime - startTime;
    cout << "omega更新時間:" << time << "\n";
    #endif

    #ifdef DEBUG
    cout << "次元重みの更新完了\n";
    #endif 

    updateAlphaNb();
}



void OmegaSom::batchAdapt(int t){
    
    #pragma omp parallel for
    for(int i = 0; i < somMap.size(); i++){
        if(somMap[i].inflDenominator > 0.0){
            for(int k = 2; k < cfg.dimensionNum; k++){
                somMap[i].weightVec[k] = somMap[i].inflNumerator[k] / somMap[i].inflDenominator;
            }
        }
    }
}

int OmegaSom::findBMU(int inputIdx){
    int bmuIdx = -1;
    double bmuDist = std::numeric_limits<double>::max();

    for(int k = 0; k < cfg.dimensionNum; k++){
        double safeOmega = max(0.0,omega[k]);
        preCalcOmega[k] = pow(safeOmega,cfg.somBeta);
    }

    //BMU探索の並列化
    #pragma omp parallel
    {

        int privateBMUIdx = -1;
        double privateBMUDist = std::numeric_limits<double>::max();

        //並列化+nowaitで終わった順にcriticalに飛ぶ
        #pragma omp for nowait
        for(size_t i = 0; i < somMap.size(); i++){
            
            double privateDist = 0;

            for(int k = 0; k < cfg.dimensionNum; k++){
                privateDist += preCalcOmega[k] * ((disasterMap[inputIdx].vec[k] - somMap[i].weightVec[k]) * (disasterMap[inputIdx].vec[k] - somMap[i].weightVec[k]));
            } 
            
            if(privateDist < privateBMUDist){
                privateBMUDist = privateDist;
                privateBMUIdx = i;
            }
        }

        //ここは並列化されず、順番に処理される
        #pragma omp critical
        {
            if(privateBMUDist < bmuDist){
                bmuDist = privateBMUDist;
                bmuIdx = privateBMUIdx;
            }
        }
    }

    return bmuIdx;
}

void OmegaSom::onlineAdapt(int BMUIdx,int inputVec){

    for(int i = 0; i < somMap.size();i++){

        //近傍関数を計算
        double nb = neighborhoodFunction(BMUIdx,i);
        
        //2σで枝刈り
        if(nb < 0.046)continue;

        //X,Y座標を変動させるかどうか 2なら変動させていない　0なら変動させてる
        for(int k = 2; k < cfg.dimensionNum;k++){
            somMap[i].weightVec[k] = somMap[i].weightVec[k] + alpha * nb * (disasterMap[inputVec].vec[k] - somMap[i].weightVec[k]);
        }
    }
}

double OmegaSom::neighborhoodFunction(int BMUIdx,int pVecIdx){
    if(BMUIdx < 0 || pVecIdx < 0){
        cout << "異常あり\n";
    }
    double numeretor = (somMap[BMUIdx].x - somMap[pVecIdx].x) * (somMap[BMUIdx].x - somMap[pVecIdx].x) + (somMap[BMUIdx].y - somMap[pVecIdx].y) * (somMap[BMUIdx].y - somMap[pVecIdx].y);
    double denominator = 2 * nbRadius * nbRadius;
    double ret = exp(-1 * numeretor / denominator);
    if(isnan(ret)){
        cerr << "nan値検出 omegaSom.cpp:158" << "\n";
        abort();
    }
    return ret;
}

void OmegaSom::updateOmega(int BMUIdx,int inputIdx,int t){
    
    //まず各D_nを求める
    fill(density.begin(),density.end(),0);

    for(int k = 0; k < somMap.size(); k++){
        double nb = neighborhoodFunction(BMUIdx,k);
        for(int n = 0; n < cfg.dimensionNum; n++){
            density[n] += nb * ((disasterMap[inputIdx].vec[n] - somMap[k].weightVec[n]) * (disasterMap[inputIdx].vec[n] - somMap[k].weightVec[n]));
        }
    }

    //omega_nを求める
    for(int n = 0; n < cfg.dimensionNum; n++){
        
        double tmp = 0;
        for(int i = 0; i < cfg.dimensionNum; i++){
            tmp += pow(((density[n] + 1e-6) / (density[i] + 1e-6)),((double)1 / (cfg.somBeta - 1)));
        }
        double newOmega = pow(tmp,-1);
        runningSum[n] -= omegaHistery[n][t % cfg.somWindowSize];
        omegaHistery[n][t % cfg.somWindowSize] = newOmega;
        runningSum[n] += newOmega;
        
        omega[n] = runningSum[n] / cfg.somWindowSize;
    }
    
    //1時刻で1000世代を超えるとnan値が出現する
    for(int n = 0; n < cfg.dimensionNum; n++) {
        if(isnan(omegaHistery[n][t % cfg.somWindowSize])){
        cerr << "nan値検出 omegaSom.cpp:198" << "\n";
        abort();
        }
    }

}

void OmegaSom::batchUpdateOmega(int t){
    //D_nを求める
    fill(density.begin(),density.end(),0);

    #pragma omp parallel
    {
        vector<double> d(cfg.dimensionNum,0.0);

        #pragma omp for nowait
        for(int j = 0; j < static_cast<int>(somMap.size()); j++){//ニューロンごとに
            
            for(int i = 0; i < static_cast<int>(disasterMap.size()); i++){//入力データのBMUの
                double nb = neighborhoodFunction(BMUIdxes[i],j);
                if(nb < 0.046)continue;

                for(int n = 0; n < cfg.dimensionNum; n++){
                    d[n] += nb * (disasterMap[i].vec[n] - somMap[j].weightVec[n]) * (disasterMap[i].vec[n] - somMap[j].weightVec[n]);
                }
            }
        }

        #pragma omp critical
        {
            for(int n = 0; n < cfg.dimensionNum; n++){
                density[n] += d[n];
            }
        }
    }

    //omega_nを求める
    for(int n = 0; n < cfg.dimensionNum; n++){
        
        double tmp = 0;
        for(int i = 0; i < cfg.dimensionNum; i++){
            tmp += pow(((density[n] + 1e-6) / (density[i] + 1e-6)),((double)1 / (cfg.somBeta - 1)));
        }
        double newOmega = pow(tmp,-1);
        runningSum[n] -= omegaHistery[n][t % cfg.somWindowSize];
        omegaHistery[n][t % cfg.somWindowSize] = newOmega;
        runningSum[n] += newOmega;
        
        omega[n] = runningSum[n] / cfg.somWindowSize;
    }
    
    //1時刻で1000世代を超えるとnan値が出現する
    for(int n = 0; n < cfg.dimensionNum; n++) {
        if(isnan(omegaHistery[n][t % cfg.somWindowSize])){
        cerr << "nan値検出 omegaSom.cpp:296" << "\n";
        abort();
        }
    }
}

//セミバッチの実装
void OmegaSom::semiBatchLearn(int time){

    #ifdef DEBUG
    double startTime = omp_get_wtime();
    #endif 

    vector<int> dataIdxes(disasterMap.size(),0);
    std::iota(dataIdxes.begin(),dataIdxes.end(),0);

    std::shuffle(dataIdxes.begin(),dataIdxes.end(),semiBatchRandEngine);

    #pragma omp parallel for
    for(int i = 0; i < cfg.batchSize; i++){
        int idx = dataIdxes[i];
        BMUIdxes[i] = findBMU(idx);
    }

    #pragma omp parallel for
    for(int i = 0; i < static_cast<int>(somMap.size()); i++){
        
        //初期化
        somMap[i].inflDenominator = 0.0;
        somMap[i].inflNumerator.assign(cfg.dimensionNum,0.0);

        //セミバッチの入力データから探索する
        for(int j = 0; j < cfg.batchSize;j++){
            double nb = neighborhoodFunction(BMUIdxes[j],i);
            if(nb < 0.046) continue;
            somMap[i].inflDenominator += nb;
            
            int actualIdx = BMUIdxes[j];
            for(int n = 0; n < cfg.dimensionNum; n++){
                somMap[i].inflNumerator[n] += nb*disasterMap[actualIdx].vec[n];
            }
        }
    }

    semiBatchAdapt(time);
    semiBatchUpdateOmega(time);
    updateAlphaNb();

    #ifdef DEBUG
    double endTime = omp_get_wtime();
    double betTime = endTime - startTime;
    cout << "セミバッチ学習時間:" << betTime << "\n";
    #endif
}

void OmegaSom::semiBatchAdapt(int time){
    
    #pragma omp parallel for
    for(int i = 0; i < somMap.size(); i++){
        if(somMap[i].inflDenominator > 0.0){
            for(int k = 2; k < cfg.dimensionNum; k++){
                double tar = somMap[i].inflNumerator[k] / somMap[i].inflDenominator;
                somMap[i].weightVec[k] = (1.0 - alpha) * somMap[i].weightVec[k] + (alpha * tar);
                
            }
        }
    }
}

void OmegaSom::semiBatchUpdateOmega(int time){
    //まず各D_nを求める
    fill(density.begin(),density.end(),0);

    #pragma omp parallel
    {
        vector<double> d(cfg.dimensionNum,0.0);

        #pragma omp for nowait
        for(int j = 0; j < static_cast<int>(somMap.size()); j++){
            
            for(int i = 0; i < cfg.batchSize; i++){
                
                double nb = neighborhoodFunction(BMUIdxes[i],j);
                if(nb < 0.046)continue;

                int actualIdx = BMUIdxes[j];
                for(int n = 0; n < cfg.dimensionNum; n++){
                    d[n] += nb * (disasterMap[i].vec[n] - somMap[actualIdx].weightVec[n]) * (disasterMap[i].vec[n] - somMap[actualIdx].weightVec[n]);
                }
            }
        }
        #pragma omp critical
        {
            for(int n = 0; n < cfg.dimensionNum; n++){
                density[n] += d[n];
            }
        }
    }

    //omega_nを求める
    for(int n = 0; n < cfg.dimensionNum; n++){
        
        double tmp = 0;
        for(int i = 0; i < cfg.dimensionNum; i++){
            tmp += pow(((density[n] + 1e-6) / (density[i] + 1e-6)),((double)1 / (cfg.somBeta - (double)1)));
        }
        double newOmega = pow(tmp,-1);
        runningSum[n] -= omegaHistery[n][time % cfg.somWindowSize];
        omegaHistery[n][time % cfg.somWindowSize] = newOmega;
        runningSum[n] += newOmega;
        
        omega[n] = runningSum[n] / cfg.somWindowSize;
    }
    
    //1時刻で1000世代を超えるとnan値が出現する
    for(int n = 0; n < cfg.dimensionNum; n++) {
        if(isnan(omegaHistery[n][time % cfg.somWindowSize])){
        cerr << "nan値検出 omegaSom.cpp:446" << "\n";
        abort();
        }
    }
}

void OmegaSom::updateAlphaNb(){//指数関数での減少スケジュール
    // alpha = max(cfg.somFinAlpha, cfg.somInitAlpha * exp(-(double)localIteration / tau));
    // nbRadius = max(cfg.somFinNbRadius, cfg.somInitNbRadius * exp(-(double)localIteration / tau));
    
    alpha = cfg.somInitAlpha * pow((cfg.somFinAlpha / cfg.somInitAlpha),((double)localIteration / cfg.somIterNum));
    nbRadius = cfg.somInitNbRadius * pow((cfg.somFinNbRadius / cfg.somInitNbRadius),((double)localIteration / cfg.somIterNum));

    localIteration++;
}

// void OmegaSom::saveNeuronState(int t){
//     ostringstream oss;
    
//     oss << cfg.csvOutputPath << "neuron_gen_" << setfill('0') << setw(6) << t << ".csv";
//     string filePath = oss.str();

//     ofstream file(filePath);
//     if (!file.is_open()) return;

//     file << "x,y,risk,isPossible\n";

//     for(int i = 0; i < somMap.size(); i++){
//         file << somMap[i].x << "," << somMap[i].y << ",";
//         double tmp = 0;
//         for(int j = 2; j < somMap[i].weightVec.size();j++){
//             tmp += somMap[i].weightVec[j];
//         }

//         file << tmp << "," << somMap[i].isPossible << "\n";
//     }
//     file.close();
// }

void OmegaSom::saveNeuronState(int t){//バイナリ書き込み
    ostringstream oss;
    oss << cfg.binOutputPath << "neuron_gen_" << setfill('0') << setw(6) << t << ".bin";
    string filePath = oss.str();

    ofstream outFile(filePath,std::ios::binary);//ファイルのオープン

    for(size_t i = 0; i < somMap.size(); i++){
        double tmp = 0;
        for(size_t j = 2; j < somMap[i].weightVec.size(); j++){
            tmp += somMap[i].weightVec[j];
        }

        if(outFile.is_open()){
            neuronState ns = {somMap[i].x,somMap[i].y,static_cast<float>(tmp),1};
            if(somMap[i].isPossible == true){
                ns.possible = 1;
            }else{
                ns.possible = 0;
            }

            outFile.write(reinterpret_cast<const char*>(&ns),sizeof(neuronState));
        }
    }
    outFile.close();
}