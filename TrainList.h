#pragma once

#include <iostream>
#include <string>
#include "TrainNode.h"

using namespace std;

class TrainList {
private:
    TrainNode* head;

public:
    TrainList() : head(NULL) {}
    ~TrainList();
    TrainNode* getHead() const { return head; }
    void addTrain(const string& code, int direction);
    TrainNode* findTrainByCode(const string& code) const;
    void addCarriageToTrain(const string& code, int cNo, const string& type, int seats);
    void addStopScheduleToTrain(const string& code, int stID, int arr, int dep, int track);
};