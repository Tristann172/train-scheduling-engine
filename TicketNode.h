#pragma once

#include "CarriageNode.h"
#include "StopScheduleNode.h"
#include <string>

class TrainNode {
private:
    std::string trainCode;
    int direction; // 1: Nam, 0: Bắc
    CarriageNode* headCarriage;
    StopScheduleNode* headSchedule;
    TrainNode* nextTrain;

public:
    TrainNode(const std::string& code = "", int dir = 1)
        : trainCode(code), direction(dir), headCarriage(nullptr), headSchedule(nullptr), nextTrain(nullptr) {}

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

    // Getters
    std::string getTrainCode() const { return trainCode; }
    int getDirection() const { return direction; }
    CarriageNode* getHeadCarriage() const { return headCarriage; }
    StopScheduleNode* getHeadSchedule() const { return headSchedule; }
    TrainNode* getNextTrain() const { return nextTrain; }

    // Setters
    void setNextTrain(TrainNode* next) { nextTrain = next; }
    void setHeadCarriage(CarriageNode* head) { headCarriage = head; }
    void setHeadSchedule(StopScheduleNode* head) { headSchedule = head; }

    // Phương thức thêm toa xe
    void addCarriage(int carriageNo, const std::string& type, int totalSeats) {
        CarriageNode* newCarriage = new CarriageNode(carriageNo, type, totalSeats);
        if (headCarriage == nullptr) {
            headCarriage = newCarriage;
        } else {
            CarriageNode* temp = headCarriage;
            while (temp->getNextCarriage() != nullptr) {
                temp = temp->getNextCarriage();
            }
            temp->setNextCarriage(newCarriage);
        }
    }

    // Phương thức thêm lịch dừng
    void addSchedule(int stationID, int arrivalMinute, int departureMinute, int trackUsed) {
        StopScheduleNode* newStop = new StopScheduleNode(stationID, arrivalMinute, departureMinute, trackUsed);
        if (headSchedule == nullptr) {
            headSchedule = newStop;
        } else {
            StopScheduleNode* temp = headSchedule;
            while (temp->getNextStop() != nullptr) {
                temp = temp->getNextStop();
            }
            temp->setNextStop(newStop);
        }
    }
};
