#include "omegaSom.h"

using namespace std;

OmegaSom::OmegaSom(const config& cfg,const vector<MapCell>& dMap): cfg(cfg),disasterMap(dMap){
    gen.seed(cfg.somSeed);
    uniform_real_distribution<double> rdist(0,1);
    

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

            somMap.push_back(move(neuron));
        }
    }

    //メモリ領域削減
    somMap.shrink_to_fit();

    double initialWeight = 1.0 / cfg.dimensionNum;
    omega.assign(cfg.dimensionNum,initialWeight);
    density.assign(cfg.dimensionNum,0);
    runningSum.assign(cfg.somWindowSize,0);

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
    beta = 2;
    tau = 20;
    resetLocalIter();
}

void OmegaSom::onlineLearn(int t){
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
    for(int j = 0; j < disasterMap.size();j++){
        int BMUIdx = findBMU(j);

    }

}

int OmegaSom::findBMU(int inputIdx){
    int bmuIdx = -1;
    double bmuDist = std::numeric_limits<double>::max();

    for(int k = 0; k < cfg.dimensionNum; k++){
        preCalcOmega[k] = pow(omega[k],beta);
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
            tmp += pow(((density[n] + 1e-6) / (density[i] + 1e-6)),((double)1 / (beta - 1)));
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
    cout << "\n";
}

void OmegaSom::updateAlphaNb(){//指数関数での減少スケジュール
    // alpha = max(cfg.somFinAlpha, cfg.somInitAlpha * exp(-(double)localIteration / tau));
    // nbRadius = max(cfg.somFinNbRadius, cfg.somInitNbRadius * exp(-(double)localIteration / tau));
    
    alpha = cfg.somInitAlpha * pow((cfg.somFinAlpha / cfg.somInitAlpha),((double)localIteration / cfg.somIterNum));
    nbRadius = cfg.somInitNbRadius * pow((cfg.somFinNbRadius / cfg.somInitNbRadius),((double)localIteration / cfg.somIterNum));

    localIteration++;
}

void OmegaSom::saveNeuronState(int t){
    ostringstream oss;
    
    oss << cfg.csvOutputPath << "neuron_gen_" << setfill('0') << setw(6) << t << ".csv";
    string filePath = oss.str();

    ofstream file(filePath);
    if (!file.is_open()) return;

    file << "x,y,risk,isPossible\n";

    for(int i = 0; i < somMap.size(); i++){
        file << somMap[i].x << "," << somMap[i].y << ",";
        double tmp = 0;
        for(int j = 2; j < somMap[i].weightVec.size();j++){
            tmp += somMap[i].weightVec[j];
        }

        // if(tmp >= 7){
        //     cout << i << "番目のニューロン" << endl;
        //     for(int n = 0; n < 7; n++){
        //         cout << somMap[i].weightVec[n] << ",";
        //     }
        //     cout << endl;

        //     cout << "omegaの値" << endl;
        //     for(auto val : omega){
        //         cout << val << ",";
        //     }
        //     cout << endl;
        //     std::cerr << "次元数を超えた値になっています" << endl;
        //     abort();
        // }


        file << tmp << "," << somMap[i].isPossible << "\n";
    }
    file.close();
}