#include "omegaSom.h"

using namespace std;

OmegaSom::OmegaSom(config& cfg,vector<MapCell>& dMap): cfg(cfg),disasterMap(dMap){
    
    gen.seed(seed);

    for(auto& cell : dMap){
        if(!cell.isRoad){
            continue;
        }


    }

    omega.assign(cfg.dimensionNum,0);
    density.assign(cfg.dimensionNum,0);

    alpha = cfg.somInitAlpha;
    nbRadius = cfg.somInitNbRadius;
}

void OmegaSom::onlineLearn(int t){
    //入力データをランダムに一つ選ぶ
    uniform_int_distribution<int> dist(0,disasterMap.size()-1);
    int inputIdx = dist(gen);
    int BMUIdx = findBMU(inputIdx); //BMU探索
    adaption(BMUIdx,inputIdx); //プロトタイプベクトル（参照ベクトル）更新
    updateOmega(BMUIdx,inputIdx); //次元重み更新
    updateAlphaNb(t); //学習率、近傍半径の更新
}

void OmegaSom::batchLearn(){

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

void OmegaSom::adaption(int BMUIdx,int inputVec){

    for(int i = 0; i < somMap.size();i++){

        //近傍関数を計算
        double nb = neighborhoodFunction(BMUIdx,i);

        for(int k = 0; k < cfg.dimensionNum-2;k++){
            somMap[i].weightVec[k] = somMap[i].weightVec[k] + alpha * nb * (disasterMap[inputVec].vec[k] - somMap[i].weightVec[k]);
        }
    }
}

double OmegaSom::neighborhoodFunction(int BMUIdx,int pVecIdx){
    double numeretor = (somMap[BMUIdx].x - somMap[pVecIdx].x) * (somMap[BMUIdx].x - somMap[pVecIdx].x) + (somMap[BMUIdx].y - somMap[pVecIdx].y) * (somMap[BMUIdx].y - somMap[pVecIdx].y);
    double denominator = 2 * nbRadius * nbRadius;
    return exp(numeretor / denominator);
}

void OmegaSom::updateOmega(int BMUIdx,int inputIdx){
    
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
        
        if(density[n] == 0){
            omega[n] = 0;
            continue;
        }else{
            double tmp = 0;
            for(int i = 0; i < cfg.dimensionNum; i++){
                tmp += pow((density[n] / density[i]),((double)1 / (beta - 1)));
            }
            omega[n] = pow(tmp,-1);
        }
    }
}

void OmegaSom::updateAlphaNb(int t){ //IterMaxは変える必要がある　なぜならiterMaxがわからないから
    alpha = cfg.somInitAlpha * pow((cfg.somFinAlpha / cfg.somInitAlpha),(t/cfg.somIterMax));
    nbRadius = cfg.somInitNbRadius * pow((cfg.somFinNbRadius / cfg.somFinNbRadius),(t/cfg.somIterMax));
}
