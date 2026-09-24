#pragma once

#include <iostream>
#include <string>
#include "TrainNode.h"

class StationList;

class TrainList {
private:
    TrainNode* head;

public:
    TrainList();
    ~TrainList();
    
    TrainNode* getHead() const;
    void addTrain(const std::string& code, int direction);
    TrainNode* findTrainByCode(const std::string& code) const;
    void addCarriageToTrain(const std::string& code, int cNo, const std::string& type, int seats);
    void addStopScheduleToTrain(const std::string& code, int stID, int arr, int dep, int track);

    void resolveConflictAndScheduleSiding(StationList* stationList);
}; 