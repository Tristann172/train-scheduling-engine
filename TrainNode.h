#ifndef TRAIN_NODE_H
#define TRAIN_NODE_H

#include <string>
#include "CarriageNode.h"
#include "StopScheduleNode.h"

class TrainNode {
private:
    std::string trainCode;
    int direction;                 
    CarriageNode* headCarriage;     
    StopScheduleNode* headSchedule; 
    TrainNode* nextTrain; 

public:
    TrainNode(const std::string& code, int dir)
        : trainCode(code), direction(dir), headCarriage(NULL), headSchedule(NULL), nextTrain(NULL) {}

    ~TrainNode() {
        while (headCarriage != NULL) {
            CarriageNode* temp = headCarriage;
            headCarriage = headCarriage->getNextCarriage();
            delete temp;
        }
        while (headSchedule != NULL) {
            StopScheduleNode* temp = headSchedule;
            headSchedule = headSchedule->getNextStop();
            delete temp;
        }
    }

    std::string getTrainCode() const { return trainCode; }
    int getDirection() const { return direction; }
    CarriageNode* getHeadCarriage() const { return headCarriage; }
    StopScheduleNode* getHeadSchedule() const { return headSchedule; }
    TrainNode* getNextTrain() const { return nextTrain; }

    void setHeadCarriage(CarriageNode* head) { headCarriage = head; }
    void setHeadSchedule(StopScheduleNode* head) { headSchedule = head; }
    void setNextTrain(TrainNode* next) { nextTrain = next; }
};

#endif
