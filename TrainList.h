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

    ~TrainList() {
        while (head != NULL) {
            TrainNode* temp = head;
            head = head->getNextTrain();
            delete temp; 
        }
    }

    TrainNode* getHead() const { return head; }

    void addTrain(const string& code, int direction) {
        TrainNode* newNode = new TrainNode(code, direction);
        if (head == NULL) {
            head = newNode;
            return;
        }
        TrainNode* temp = head;
        while (temp->getNextTrain() != NULL) {
            temp = temp->getNextTrain();
        }
        temp->setNextTrain(newNode);
    }

    TrainNode* findTrainByCode(const string& code) const {
        TrainNode* temp = head;
        while (temp != NULL) {
            if (temp->getTrainCode() == code) return temp;
            temp = temp->getNextTrain();
        }
        return NULL;
    }

    void addCarriageToTrain(const string& code, int cNo, const string& type, int seats) {
        TrainNode* train = findTrainByCode(code);
        if (!train) return;

        CarriageNode* newCarriage = new CarriageNode(cNo, type, seats);
        if (train->getHeadCarriage() == NULL) {
            train->setHeadCarriage(newCarriage);
            return;
        }
        CarriageNode* temp = train->getHeadCarriage();
        while (temp->getNextCarriage() != NULL) {
            temp = temp->getNextCarriage();
        }
        temp->setNextCarriage(newCarriage);
    }

    void addStopScheduleToTrain(const string& code, int stID, int arr, int dep, int track) {
        TrainNode* train = findTrainByCode(code);
        if (!train) return;

        StopScheduleNode* newStop = new StopScheduleNode(stID, arr, dep, track);
        if (train->getHeadSchedule() == NULL) {
            train->setHeadSchedule(newStop);
            return;
        }
        StopScheduleNode* temp = train->getHeadSchedule();
        while (temp->getNextStop() != NULL) {
            temp = temp->getNextStop();
        }
        temp->setNextStop(newStop);
    }
};