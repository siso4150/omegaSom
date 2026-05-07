#pragma once

#include "config.h"
#include "disasterMap.h"

#include <vector>
#include <random>
#include <math.h>

using namespace std;

struct Neuron{
    vector<double> weightVec;
    int x,y;
};

class OmegaSom{

private:

    double alpha; //学習率
    double nbRadius; //現在の近接半径σ
    double beta; //次元重みに掛けるパラメタ
    
    vector<Neuron> somMap; //somマップ格納場所
    vector<double> omega; //次元ごとの重み
    vector<double> density; //次元ごとの密集具合

    config& cfg; //コンフィグ用参照
    vector<MapCell>& disasterMap; //災害マップ保持用の参照

    unsigned int seed = 30;
    std::mt19937 gen;



public:
    OmegaSom(config&,vector<MapCell>&);

    void onlineLearn(int); //オンライン学習

    void batchLearn(); //バッチ学習

    int findBMU(int); //BMUを見つける

    void adaption(int,int); //適応過程　参照ベクトルの値を更新

    void updateOmega(int,int); //次元重みを更新
    void updateAlphaNb(int); //学習率・近傍半径の更新

    double neighborhoodFunction(int,int); //近傍関数
    double calcNeuronDist(int,int); //両ノードの距離を計算

};