#pragma once

#include "config.h"
#include "disasterMap.h"
#include "acoData.h"

#include <vector>
#include <random>
#include <math.h>
#include <memory>
#include <omp.h>



using namespace std;

struct Neuron{
    vector<double> weightVec;
    
    bool isPossible; //通行可能かどうか
    int x,y;
    double riskval;

    vector<double> inflNumerator;
    double inflDenominator;
    int bmuIdx;

    unique_ptr<AcoData> acoData;
};

struct neuronState{//ニューロン状態をバイナリ出力するための構造体
    int32_t x;
    int32_t y;
    float riskSum;
    int32_t possible;
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

    vector<double> preCalcOmega; //オメガの計算用
    

    const config& cfg; //コンフィグ用参照
    const vector<MapCell>& disasterMap; //災害マップ保持用の参照
    vector<int> BMUIdxes; //各入力データのBMU保存配列

    unsigned int seed = 50;
    std::mt19937 gen;

    int localIteration;



public:
    OmegaSom(const config&,const vector<MapCell>&);

    //オンライン型学習
    void onlineLearn(int); //オンライン学習
    void onlineAdapt(int,int); //適応過程　参照ベクトルの値を更新
    void updateOmega(int,int,int); //次元重みを更新

    //バッチ型学習
    void batchLearn(int); //バッチ学習
    void batchCoop(int); //協調過程
    void batchAdapt(int); //適応過程
    void batchUpdateOmega(int); //次元重みを更新（バッチ型）

    //セミバッチ型学習
    void semiBatchLearn(int);
    void semiBatchAdapt(int);
    void semiBatchUpdateOmega(int);


    //共通関数
    int findBMU(int); //BMUを見つける
    void saveNeuronState(int); //ニューロン状態を保存
    void updateAlphaNb(); //学習率・近傍半径の更新
    double neighborhoodFunction(int,int); //近傍関数
    double calcNeuronDist(int,int); //両ノードの距離を計算

    void resetLocalIter(){localIteration = 1;};

    vector<Neuron>& getSomMap(){return somMap;};
    
    
    void setIsPossible(int idx,bool possible){
        somMap.at(idx).isPossible = possible;
    }

};