#include "omegaSom.h"

using namespace std;

OmegaSom::OmegaSom(const config& cfg,const vector<MapCell>& dMap): cfg(cfg),disasterMap(dMap){
    
    gen.seed(seed);
    uniform_real_distribution<double> rdist(0,1);

    for(int i = 0; i < disasterMap.size();i++){
        if(disasterMap[i].isRoad == false){
            continue;
        }else{
            Neuron neuron;
            neuron.x = disasterMap[i].x;
            neuron.y = disasterMap[i].y;

            for(int n = 0; n < cfg.dimensionNum; n++){
                neuron.weightVec.push_back(rdist(gen));
            }
            neuron.weightVec[0] = neuron.x;
            neuron.weightVec[1] = neuron.y;

            somMap.push_back(neuron);
        }
    }

    omega.assign(cfg.dimensionNum,(double)1 / cfg.dimensionNum);
    density.assign(cfg.dimensionNum,0);

    cout << "0世代目" << endl;
    for(auto val : omega) cout << val << " ";
    cout << endl;

    for(int n = 0; n < cfg.dimensionNum; n++){
        omegaHistery.push_back(vector<double>(cfg.somWindowSize,0.0));
    }
    runningSum.assign(cfg.dimensionNum,0);

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


    for(int i = 0; i < somMap.size(); i++){
        double dist = 0;
        for(int k = 0; k < cfg.dimensionNum; k++){
            //とりあえず、全部の重みを掛ける
            dist += pow(omega[k],beta) * ((disasterMap[inputIdx].vec[k] - somMap[i].weightVec[k]) * (disasterMap[inputIdx].vec[k] - somMap[i].weightVec[k]));
        }
        if(dist < bmuDist){
            bmuDist = dist;
            bmuIdx = i;
        }
    }

    return bmuIdx;
}

void OmegaSom::onlineAdapt(int BMUIdx,int inputVec){

    for(int i = 0; i < somMap.size();i++){

        //近傍関数を計算
        double nb = neighborhoodFunction(BMUIdx,i);

        for(int k = 0; k < cfg.dimensionNum;k++){
            somMap[i].weightVec[k] = somMap[i].weightVec[k] + alpha * nb * (disasterMap[inputVec].vec[k] - somMap[i].weightVec[k]);
        }
    }
}

double OmegaSom::neighborhoodFunction(int BMUIdx,int pVecIdx){
    double numeretor = (somMap[BMUIdx].x - somMap[pVecIdx].x) * (somMap[BMUIdx].x - somMap[pVecIdx].x) + (somMap[BMUIdx].y - somMap[pVecIdx].y) * (somMap[BMUIdx].y - somMap[pVecIdx].y);
    double denominator = 2 * nbRadius * nbRadius;
    double ret = exp(-1 * numeretor / denominator);
    if(isnan(ret)){
        cerr << "nan値検出" << endl;
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

    // cout << "Dn : ";
    // for(auto val : density) {
    //     cout << val << " ";
    //     if(isnan(val)){
    //     cerr << "nan値検出" << endl;
    //     abort();
    //         }
    // }
    // cout << endl;

    //omega_nを求める
    for(int n = 0; n < cfg.dimensionNum; n++){
        
        if(density[n] == 0){
            runningSum[n] -= omegaHistery[n][t % cfg.somWindowSize];
            omegaHistery[n][t % cfg.somWindowSize] = 0;
            continue;
        }else{
            double tmp = 0;
            for(int i = 0; i < cfg.dimensionNum; i++){
                tmp += pow((density[n] / density[i]),((double)1 / (beta - 1)));
            }
            double newOmega = pow(tmp,-1);
            runningSum[n] -= omegaHistery[n][t % cfg.somWindowSize];
            omegaHistery[n][t % cfg.somWindowSize] = newOmega;
            runningSum[n] += newOmega;
        }
        omega[n] = runningSum[n] / cfg.somWindowSize;
    }
    cout << "omegaHistery,";
    for(int n = 0; n < cfg.dimensionNum; n++) {
        cout << omegaHistery[n][t % cfg.somWindowSize] << ",";
        if(isnan(omegaHistery[n][t % cfg.somWindowSize])){
        cerr << "nan値検出" << endl;
        abort();
    }
    }
    cout << endl;
}

void OmegaSom::updateAlphaNb(){//指数関数での減少スケジュール
    alpha = max(cfg.somFinAlpha, cfg.somInitAlpha * exp(-(double)localIteration / tau));
    nbRadius = max(cfg.somFinNbRadius, cfg.somInitNbRadius * exp(-(double)localIteration / tau));
    localIteration++;
}

void OmegaSom::saveNeuronState(int t){
    ostringstream oss;
    
    oss << cfg.csvOutputPath << "neuron_gen_" << setfill('0') << setw(4) << t << ".csv";
    string filePath = oss.str();

    ofstream file(filePath);
    if (!file.is_open()) return;

    file << "x,y,risk" << endl;

    for(int i = 0; i < somMap.size(); i++){
        file << somMap[i].x << "," << somMap[i].y << ",";
        double tmp = 0;
        for(int j = 2; j < somMap[i].weightVec.size();j++){
            tmp += somMap[i].weightVec[j];
        }
        file << tmp << endl;
    }
    file.close();
}