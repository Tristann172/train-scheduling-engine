#pragma once

#include <string>
#include "CarriageNode.h"
#include "StopScheduleNode.h"

class TrainNode {
private:
    std::string trainCode;
    int direction;                 // 0: Bắc -> Nam, 1: Nam -> Bắc
    int priority;                  // Mức ưu tiên (vd: SE1 ưu tiên cao hơn tàu hàng)
    int delayMinutes;              // Độ trễ tích lũy (phút)
    CarriageNode* headCarriage;     
    StopScheduleNode* headSchedule; 
    TrainNode* nextTrain; 

public:
    TrainNode(const std::string& code, int dir, int prio = 1)
        : trainCode(code), direction(dir), priority(prio), delayMinutes(0),
          headCarriage(nullptr), headSchedule(nullptr), nextTrain(nullptr) {}

    ~TrainNode() {
        while (headCarriage != nullptr) {
            CarriageNode* temp = headCarriage;
            headCarriage = headCarriage->getNextCarriage();
            delete temp;
        }
        while (headSchedule != nullptr) {
            StopScheduleNode* temp = headSchedule;
            headSchedule = headSchedule->getNextStop();
            delete temp;
        }
    }

    std::string getTrainCode() const { return trainCode; }
    int getDirection() const { return direction; }
    int getPriority() const { return priority; }
    int getDelayMinutes() const { return delayMinutes; }
    CarriageNode* getHeadCarriage() const { return headCarriage; }
    StopScheduleNode* getHeadSchedule() const { return headSchedule; }
    TrainNode* getNextTrain() const { return nextTrain; }

    void setPriority(int prio) { priority = prio; }
    void setDelayMinutes(int delay) { delayMinutes = delay; }
    void setHeadCarriage(CarriageNode* head) { headCarriage = head; }
    void setHeadSchedule(StopScheduleNode* head) { headSchedule = head; }
    void setNextTrain(TrainNode* next) { nextTrain = next; }
};