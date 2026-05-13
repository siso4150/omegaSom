#pragma once

#include "config.h"

#include <vector>
#include <string>
#include <sstream>
#include <istream>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <math.h>

using namespace std;

struct MapCell{
    int x;
    int y;
    vector<double> vec;
    bool isRoad;
};

class DisasterMap{

private:
    vector<MapCell> disasterMap;
    const config& cfg;
    double normalize(double x,double xmin,double xmax){return (x-xmin) / (xmax-xmin);};

    std::string trim(const std::string& str) {
        std::string s = str;
        // 改行文字 (\r, \n) やスペースを削除
        s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
        s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
        s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
        return s;
    }

    double parseDouble(const std::string& str) {
        std::string s = trim(str);
        if (s.empty() || s == "NULL") return 0.0;
        try {
            return std::stod(s);
        } catch (...) {
            return 0.0;
        }
    }

public:
    DisasterMap(const config&);
    void loadFromCsv(const string&);
    void updateMap();
    void vecNormalize();
    const vector<MapCell>& getDisasterMap() const {return disasterMap;};
    

};