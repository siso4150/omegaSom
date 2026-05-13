#include "disasterMap.h"

DisasterMap::DisasterMap(const config& cfg) : cfg(cfg){

}

void DisasterMap::loadFromCsv(const string& filePath){
    ifstream file(filePath);
    if (!file.is_open()){
        cout << filePath << "を読み込めませんでした" << endl;
        abort();
    }

    disasterMap.clear();

    string line;
    getline(file,line); //ヘッダ行飛ばし

    while (std::getline(file, line)) {

        stringstream ss(line);
        string item;
        MapCell cell;

        //道路かどうか
        getline(ss, item, ',');
        item = trim(item);
        cell.isRoad = !item.empty();

        //x座標
        getline(ss, item, ',');
        item = trim(item);
        cell.x = stoi(item);
        cell.vec.push_back(stoi(item));

        //y座標
        getline(ss, item, ',');
        item = trim(item);
        cell.y = stoi(item);
        cell.vec.push_back(stoi(item));

        //標高
        getline(ss, item, ',');
        cell.vec.push_back(parseDouble(item));

        //リスク値（津波）
        getline(ss, item, ',');
        cell.vec.push_back(parseDouble(item));

        disasterMap.push_back(cell);
    }

    vecNormalize();
}

void DisasterMap::updateMap(){

}

void DisasterMap::vecNormalize(){

    for(int n = 0; n < cfg.dimensionNum; n++){
        double max = 0;
        double min = 1e9;
        for(int i = 0; i < disasterMap.size();i++){
            max = std::max(disasterMap[i].vec[n],max);
            min = std::min(disasterMap[i].vec[n],min);
        }
        for(int i = 0; i < disasterMap.size();i++){
            disasterMap[i].vec[n] = normalize(disasterMap[i].vec[n],min,max);
        }
    }
}


