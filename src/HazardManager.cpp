
#include "HazardManager.h"

HazardManager::HazardManager(const config& cfg,OmegaSom* map, Colony* table) : cfgRef(cfg),somPtr(map),colonyPtr(table){
    gen.seed(42);
}

void HazardManager::randomRoadClose(){

    //数回、道路を封鎖する 
    //ゴールが塞がれるとまずいので、修正する
    for(int i = 0; i < 2; i++){
        uniform_int_distribution<> dist(0,somPtr->getSomMap().size()-1);
        int targetIdx = dist(gen);
        somPtr->setIsPossible(targetIdx,false);
        

        //周囲数マスも封鎖する
        int x = somPtr->getSomMap().at(targetIdx).x;
        int y = somPtr->getSomMap().at(targetIdx).y;
        for(int i = 0; i < 8; i++){
            int movedX = x + dX[i];
            int movedY = y + dY[i];
            

            if((movedX >= 0 && movedY >= 0 && movedX < cfgRef.mapCol && 
                movedY < cfgRef.mapRow && colonyPtr->getNeuronIdxTable()[movedY][movedX] >= 0)){//範囲内かつ、道路(変換テーブルに座標が入る)

                int movedIdx = colonyPtr->getNeuronIdxTable()[movedY][movedX];
                somPtr->setIsPossible(movedIdx,false);
            }
        }
    }

}
