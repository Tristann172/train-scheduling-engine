#ifndef TRAIN_LIST_H
#define TRAIN_LIST_H

#include <iostream>
#include <string>
#include "TrainNode.h"

using namespace std;

class TrainList {
private:
    TrainNode* head;

public:
    TrainList() : head(nullptr) {}

    ~TrainList() {
        while (head != nullptr) {
            TrainNode* temp = head;
            head = head->getNextTrain();
            delete temp; // TrainNode destructor se tu dong delete headCarriage & headSchedule
        }
    }

    TrainNode* getHead() const { return head; }

    void addTrain(const string& code, int direction) {
        TrainNode* newNode = new TrainNode(code, direction);
        if (head == nullptr) {
            head = newNode;
            return;
        }
        TrainNode* temp = head;
        while (temp->getNextTrain() != nullptr) {
            temp = temp->getNextTrain();
        }
        temp->setNextTrain(newNode);
    }

    TrainNode* findTrainByCode(const string& code) const {
        TrainNode* temp = head;
        while (temp != nullptr) {
            if (temp->getTrainCode() == code) return temp;
            temp = temp->getNextTrain();
        }
        return nullptr;
    }

    void addCarriageToTrain(const string& code, int cNo, const string& type, int seats) {
        TrainNode* train = findTrainByCode(code);
        if (!train) return;

        CarriageNode* newCarriage = new CarriageNode(cNo, type, seats);
        if (train->getHeadCarriage() == nullptr) {
            train->setHeadCarriage(newCarriage);
            return;
        }
        CarriageNode* temp = train->getHeadCarriage();
        while (temp->getNextCarriage() != nullptr) {
            temp = temp->getNextCarriage();
        }
        temp->setNextCarriage(newCarriage);
    }

    void addStopScheduleToTrain(const string& code, int stID, int arr, int dep, int track) {
        TrainNode* train = findTrainByCode(code);
        if (!train) return;

        StopScheduleNode* newStop = new StopScheduleNode(stID, arr, dep, track);
        if (train->getHeadSchedule() == nullptr) {
            train->setHeadSchedule(newStop);
            return;
        }
        StopScheduleNode* temp = train->getHeadSchedule();
        while (temp->getNextStop() != nullptr) {
            temp = temp->getNextStop();
        }
        temp->setNextStop(newStop);
    }
};

#endif
