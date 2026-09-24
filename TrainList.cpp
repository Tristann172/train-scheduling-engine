#include <iostream>
#include <string>
#include "TrainList.h"

using namespace std;

void TrainList::resolveConflictAndScheduleSiding(StationList* stationList) {
    if (head == NULL || stationList == NULL) return;

    TrainNode* trainA = head;
    
    while (trainA != NULL) {
        TrainNode* trainB = trainA->getNextTrain();
        
        while (trainB != NULL) {
            if (trainA->getDirection() != trainB->getDirection()) {
                
                StopScheduleNode* schedA = trainA->getHeadSchedule();
                while (schedA != NULL && schedA->getNextStop() != NULL) {
                    
                    StopScheduleNode* schedB = trainB->getHeadSchedule();
                    while (schedB != NULL && schedB->getNextStop() != NULL) {
                        
                        if (schedA->getStationID() == schedB->getNextStop()->getStationID() &&
                            schedA->getNextStop()->getStationID() == schedB->getStationID()) {
                            
                            int depA = schedA->getDepartureMinute();
                            int arrA = schedA->getNextStop()->getArrivalMinute();
                            int depB = schedB->getDepartureMinute();
                            int arrB = schedB->getNextStop()->getArrivalMinute();
                            
                            int maxDep = (depA > depB) ? depA : depB;
                            int minArr = (arrA < arrB) ? arrA : arrB;
                            
                            if (maxDep < minArr) {
                                StationNode* stA = stationList->findStationByID(schedA->getStationID());
                                
                                if (stA != NULL && stA->getNumberOfTracks() > 1) {
                                    schedA->setTrackUsed(2);
                                    schedA->setDepartureMinute(arrB + 5); 
                                }
                            }
                        }
                        schedB = schedB->getNextStop();
                    }
                    schedA = schedA->getNextStop();
                }
            }
            trainB = trainB->getNextTrain();
        }
        trainA = trainA->getNextTrain();
    }
}

TrainList::~TrainList() {
    while (head != NULL) {
        TrainNode *temp = head;
        head = head->getNextTrain();
        delete temp;
    }
}

void TrainList::addTrain(const string& code, int direction) {
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

TrainNode* TrainList::findTrainByCode(const string& code) const {
    TrainNode *temp = head;
    while (temp != NULL) {
        if (temp->getTrainCode() == code) return temp;
        temp = temp->getNextTrain();
    }
    return NULL;
}

void TrainList::addCarriageToTrain(const string& code, int cNo, const string& type, int seats) {
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

void TrainList::addStopScheduleToTrain(const string& code, int stID, int arr, int dep, int track) {
    TrainNode* train = findTrainByCode(code);
    if (!train) return;

    StopScheduleNode* newStop = new StopScheduleNode(stID, arr, dep, track);
    if (train->getHeadSchedule() == NULL) {
        train->setHeadSchedule(newStop);
        return;
    }
    StopScheduleNode* temp = train->getHeadSchedule();
    while(temp->getNextStop() != NULL) {
        temp = temp->getNextStop();
    }
    temp->setNextStop(newStop);
}
