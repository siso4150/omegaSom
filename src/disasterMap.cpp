#include "disasterMap.h"

DisasterMap::DisasterMap(const config& cfg) : cfg(cfg){
    totalTime = 24;
    allWeather.resize(totalTime);
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
        double roadWidth;
        if(!item.empty()){
            roadWidth = stoi(item);
        }else{
            roadWidth = 0;
        }

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

        //道路の幅を入れる
        cell.vec.push_back(roadWidth);

        //標高
        getline(ss, item, ',');
        cell.vec.push_back(parseDouble(item));

        disasterMap.push_back(cell);
    }

    vecNormalize();
}

void DisasterMap::loadDynamicData(){
    int pointNum = disasterMap.size();
    
    for(int t = 0; t < totalTime; t++){
        string fileName = "/home/sakai/cppfile/omegaSOM/csv/weather_" + to_string(t) + ".csv";

        ifstream ifs(fileName);

        if(!ifs.is_open()){
            std::cerr << fileName << "を開けませんでした" << endl;
        }else{
            std::cout << fileName <<  "読み込み" <<  endl;
        }

        string line;
        getline(ifs,line);

        allWeather[t].reserve(pointNum);

        while(getline(ifs,line)){
            stringstream ss(line);
            string rainStr, windStr, tempStr;
            getline(ss, rainStr, ',');
            getline(ss, windStr, ',');
            getline(ss, tempStr, ',');

            vector<double> mesh;
            //小数点の情報は落ちてる
            mesh.push_back(std::stod(rainStr));
            mesh.push_back(std::stod(windStr));
            mesh.push_back(std::stod(tempStr));

            // for(auto e : mesh){
            //     cout << e <<  ",";
            // }
            // cout << endl;

            allWeather[t].push_back(mesh);
        }
    }

    combineData();
}

void DisasterMap::combineData(){//静的データと動的データをくっつける
    for(int i = 0; i < disasterMap.size();i++){
        for(int j = 0; j < 3; j++){
            disasterMap[i].vec.push_back(allWeather[0][i][j]);
        }
    }
    cout << "combineData終了" <<  endl;
}

void DisasterMap::updateData(int time){
    for(int i = 0; i < disasterMap.size();i++){
        for(int j = 0; j < 3; j++){
            disasterMap[i].vec[cfg.staticDimensionNum+j] = allWeather[time][i][j];
        }
    }
    vecNormalizeDynamic();
}

void DisasterMap::vecNormalize(){

    for(int n = 0; n < cfg.staticDimensionNum; n++){
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

void DisasterMap::vecNormalizeDynamic(){
    for(int n = cfg.staticDimensionNum; n < cfg.dynamicDimensionNum; n++){
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



