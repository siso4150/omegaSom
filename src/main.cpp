#include "config.h"
#include "disasterMap.h"
#include "omegaSom.h"
#include "json.hpp"
#include "colony.h"
#include "HazardManager.h"

#include <fstream>
#include <iostream>


using namespace std;
using json = nlohmann::json;

int main(int argc, char* argv[]){
    
    string config_file = "json/sample.json";
    if(argc >= 2){//コマンドラインにjsonが指定されている時
        config_file = argv[1];
    }

    //ファイルの読み込み
    ifstream f("/home/sakai/cppfile/omegaSOM/json/sample.json");
    if (!f.is_open()) {
        cerr << "Error: Failed to open " << config_file << endl;
        return 1;
    }

    json data = json::parse(f);
    config cfg = data.get<config>(); //構造体へ代入(config.hppでマクロ使用)

    DisasterMap dMap(cfg);
    cout << "csvデータの読み込み中" << endl;
    dMap.loadFromCsv("/home/sakai/cppfile/omegaSOM/csv/mesh_base.csv");
    dMap.loadDynamicData();

    OmegaSom som(cfg,dMap.getDisasterMap());

    Colony colony(cfg,som.getSomMap());

    //HazardManager hazardManager(cfg,&som,&colony);

    cout << "初期化終了" << endl;

    

    int csvCnt = 1;
    for(int time = 1; time <= cfg.somIterMax; time++){
        cout << time << "世代目,";
        
        som.onlineLearn(time);
        som.saveNeuronState(time);

        if(time % 20 == 0 && csvCnt < 7){

            //colony.run();
            
            // cout << "Read :" << targetPath << endl;
            
            //マップの更新
            dMap.updateData(csvCnt);
            csvCnt++;

            som.resetLocalIter();
            //塞ぐ
            //hazardManager.randomRoadClose();
        }
    }
    colony.run();
}