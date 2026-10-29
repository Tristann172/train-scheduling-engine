#pragma once

#include <string>
#include "CarriageNode.h"
#include "StopScheduleNode.h"

class TrainNode {
private:
    std::string trainCode;
    int direction; // 1: Nam, 0: Bac
    CarriageNode* headCarriage;
    StopScheduleNode* headSchedule;
    TrainNode* nextTrain;

public:
    TrainNode(const std::string& code, int dir)
        : trainCode(code), direction(dir), headCarriage(nullptr), headSchedule(nullptr), nextTrain(nullptr) {}

    ~TrainNode() {
        while (headCarriage) {
            CarriageNode* temp = headCarriage;
            headCarriage = headCarriage->getNextCarriage();
            delete temp;
        }
        while (headSchedule) {
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

    void addCarriage(int cNo, const std::string& type, int seats) {
        CarriageNode* newNode = new CarriageNode(cNo, type, seats);
        if (!headCarriage) {
            headCarriage = newNode;
            return;
        }
        CarriageNode* temp = headCarriage;
        while (temp->getNextCarriage()) temp = temp->getNextCarriage();
        temp->setNextCarriage(newNode);
    }

    void addSchedule(int stID, int arr, int dep, int track) {
        StopScheduleNode* newNode = new StopScheduleNode(stID, arr, dep, track);
        if (!headSchedule) {
            headSchedule = newNode;
            return;
        }
        StopScheduleNode* temp = headSchedule;
        while (temp->getNextStop()) temp = temp->getNextStop();
        temp->setNextStop(newNode);
    }

    // 1. Lan truyền dời giờ các ga tiếp theo khi xảy ra trễ
    bool applyDelayPropagation(int fromStationID, int delayMinutes) {
        StopScheduleNode* curr = headSchedule;
        bool found = false;

        while (curr != nullptr) {
            if (curr->getStationID() == fromStationID) {
                found = true;
            }
            if (found) {
                curr->setArrivalMinute(curr->getArrivalMinute() + delayMinutes);
                curr->setDepartureMinute(curr->getDepartureMinute() + delayMinutes);
            }
            curr = curr->getNextStop();
        }
        return found;
    }

    int getCarriageSeats(int carriageNo) const {
        CarriageNode* c = headCarriage;
        while (c) {
            if (c->getCarriageNo() == carriageNo) return c->getTotalSeats();
            c = c->getNextCarriage();
        }
        return 0;
    }
};
