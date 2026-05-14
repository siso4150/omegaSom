#include "config.h"
#include "disasterMap.h"
#include "omegaSom.h"
#include "json.hpp"

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
    ifstream f(config_file);
    if (!f.is_open()) {
        cerr << "Error: Failed to open " << config_file << endl;
        return 1;
    }

    json data = json::parse(f);
    config cfg = data.get<config>(); //構造体へ代入(config.hppでマクロ使用)

    DisasterMap dMap(cfg);
    dMap.loadFromCsv("/home/sakai/cppfile/omegaSOM/csv/hazard_0001.csv");

    OmegaSom som(cfg,dMap.getDisasterMap());

    int csvCnt = 2;
    for(int time = 1; time <= cfg.somIterMax; time++){
        cout << time << "世代目,";
        
        som.onlineLearn(time);

        if(time % 20 == 0 && csvCnt < 7){
            ostringstream oss;
            oss << cfg.csvDirPath <<  "hazard_" << setfill('0') << setw(4) << csvCnt << ".csv";
            string targetPath = oss.str();

            // cout << "Read :" << targetPath << endl;
            dMap.loadFromCsv(targetPath);
            csvCnt++;
            som.resetLocalIter();
        }

        som.saveNeuronState(time);
    }
}