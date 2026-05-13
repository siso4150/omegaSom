#include "disasterMap.h"

void DisasterMap::loadFromCsv(const string& filePath){
    ifstream file(filePath);
    if (!file.is_open()) return;

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
    
}

void DisasterMap::updateMap(){

}


