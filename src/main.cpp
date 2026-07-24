#include "config.h"
#include "disasterMap.h"
#include "omegaSom.h"
#include "json.hpp"
#include "colony.h"
#include "HazardManager.h"

#include <fstream>
#include <iostream>
#include <filesystem>

#define DEBUG


using namespace std;
using json = nlohmann::json;

int main(int argc, char* argv[]){
    
    string config_file = "/home/sakai/cppfile/omegaSOM/json/sample.json";
    if(argc >= 2){//コマンドラインにjsonが指定されている時
        config_file = argv[1];
    }

    //ファイルの読み込み
    ifstream f(config_file);
    if (!f.is_open()) {
        cerr << "Error: Failed to open " << config_file << "\n";
        return 1;
    }

    json data = json::parse(f);
    config cfg = data.get<config>(); //構造体へ代入(Bconfig.hppでマクロ使用)

    //フォルダ作成
    std::filesystem::create_directories(cfg.binOutputPath);
    std::filesystem::create_directories(cfg.binOutputRoutePath);
    std::filesystem::create_directories(cfg.binOutputParamPath);

    DisasterMap dMap(cfg);
    cout << "csvデータの読み込み中" << "\n";
    dMap.loadFromCsv("/home/sakai/cppfile/omegaSOM/csv/mesh_base.csv");
    dMap.loadDynamicData();

    OmegaSom som(cfg,dMap.getDisasterMap());

    Colony colony(cfg,som.getSomMap());

    //HazardManager hazardManager(cfg,&som,&colony);

    cout << "初期化終了" << "\n";

    

    int csvCnt = 1;
    for(int time = 1; time <= cfg.somIterMax; time++){

        #ifdef DEBUG
            cout << "som :  "<< time << "世代目\n";
        #endif // DEBUG

        if(cfg.trainMode == 0){//Modeが0ならバッチ、1ならオンライン
            som.batchLearn(time);
        }else{
            som.onlineLearn(time);
        }
        
        som.saveNeuronState(time);
        
        if(time % cfg.somIterNum == 0 && csvCnt < 24){

            colony.run();
            
            //マップの更新
            cout << "マップの更新:";
            dMap.updateData(csvCnt);
            csvCnt++;

            som.resetLocalIter();
            //塞ぐ
            //hazardManager.randomRoadClose();
        }
    }
    colony.run();
    colony.resultParam();

    cout << "全ての処理が終了" << "\n";
}