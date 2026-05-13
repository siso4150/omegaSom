#pragma once

#include "config.h"
#include "disasterMap.h"

#include <vector>
#include <random>
#include <math.h>


using namespace std;

struct Neuron{
    vector<double> weightVec;
    vector<double> inflNumerator;
    double inflDenominator;
    int x,y;
};

class OmegaSom{

private:

    double alpha; //学習率
    double nbRadius; //現在の近接半径σ
    double beta; //次元重みに掛けるパラメタ
    double tau; //近傍半径の縮小率
    
    vector<Neuron> somMap; //somマップ格納場所
    vector<double> omega; //次元ごとの重み
    vector<double> density; //次元ごとの密集度
    vector<vector<double>> omegaHistery; //次元重みの履歴
    vector<double> runningSum; //次元重みの計算置き場
    

    const config& cfg; //コンフィグ用参照
    const vector<MapCell>& disasterMap; //災害マップ保持用の参照

    unsigned int seed = 30;
    std::mt19937 gen;



public:
    OmegaSom(const config&,const vector<MapCell>&);

    void onlineLearn(int); //オンライン学習
    void onlineAdapt(int,int); //適応過程　参照ベクトルの値を更新

    void batchLearn(int); //バッチ学習
    void batchCoop(int); //協調過程
    void batchAdapt(int,int); //適応過程

    int findBMU(int); //BMUを見つける

    

    void updateOmega(int,int,int); //次元重みを更新
    void updateAlphaNb(int); //学習率・近傍半径の更新

    double neighborhoodFunction(int,int); //近傍関数
    double calcNeuronDist(int,int); //両ノードの距離を計算

};