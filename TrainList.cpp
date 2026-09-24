#include <iostream>
#include <string>
#include "TrainList.h"
#include "StationList.h"
#include "StationNode.h"

using namespace std;

TrainList::TrainList() : head(nullptr) {}

TrainList::~TrainList() {
    clear();
}

void TrainList::clear() {
    while (head != nullptr) {
        TrainNode* temp = head;
        head = head->getNextTrain();
        delete temp;
    }
    head = nullptr;
}

TrainNode* TrainList::getHead() const {
    return head;
}

void TrainList::addTrain(const string& code, int direction) {
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

TrainNode* TrainList::findTrainByCode(const string& code) const {
    TrainNode* temp = head;
    while (temp != nullptr) {
        if (temp->getTrainCode() == code) return temp;
        temp = temp->getNextTrain();
    }
    return nullptr;
}

void TrainList::addCarriageToTrain(const string& code, int cNo, const string& type, int seats) {
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

void TrainList::addStopScheduleToTrain(const string& code, int stID, int arr, int dep, int track) {
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

// THUẬT TOÁN ĐIỀU PHỐI TRÁNH TÀU RAY ĐƠN (MEET & PASS)
void TrainList::resolveConflictAndScheduleSiding(StationList* stationList) {
    if (head == nullptr || stationList == nullptr) return;

    TrainNode* trainA = head;
    while (trainA != nullptr) {
        TrainNode* trainB = trainA->getNextTrain();
        while (trainB != nullptr) {
            // Chỉ kiểm tra khi 2 đoàn tàu chạy ngược chiều nhau
            if (trainA->getDirection() != trainB->getDirection()) {
                StopScheduleNode* sA1 = trainA->getHeadSchedule();

                while (sA1 != nullptr && sA1->getNextStop() != nullptr) {
                    StopScheduleNode* sA2 = sA1->getNextStop();
                    StopScheduleNode* sB1 = trainB->getHeadSchedule();

                    while (sB1 != nullptr && sB1->getNextStop() != nullptr) {
                        StopScheduleNode* sB2 = sB1->getNextStop();

                        // Kiểm tra 2 tàu có cùng đi vào 1 khu gian ray đơn (Ga 1 <-> Ga 2)
                        if (sA1->getStationID() == sB2->getStationID() &&
                            sA2->getStationID() == sB1->getStationID()) {

                            int depA = sA1->getDepartureMinute();
                            int arrA = sA2->getArrivalMinute();
                            int depB = sB1->getDepartureMinute();
                            int arrB = sB2->getArrivalMinute();

                            // Kiểm tra khoảng thời gian chiếm dụng khu gian có giao nhau không
                            int maxDep = (depA > depB) ? depA : depB;
                            int minArr = (arrA < arrB) ? arrA : arrB;

                            if (maxDep < minArr) {
                                // XUNG ĐỘT KHU GIAN PHÁT SINH
                                StationNode* stA = stationList->findStationByID(sA1->getStationID());
                                StationNode* stB = stationList->findStationByID(sB1->getStationID());

                                // Xét ưu tiên: Nếu tàu A nhường (mặc định hoặc A có ưu tiên thấp hơn B)
                                bool trainA_Waits = true;
                                if (stA != nullptr && stA->getNumberOfTracks() <= 1 && stB != nullptr && stB->getNumberOfTracks() > 1) {
                                    trainA_Waits = false; // Ga A ray đơn, bắt buộc tàu B phải nhường tại ga B
                                }

                                if (trainA_Waits && stA != nullptr && stA->getNumberOfTracks() > 1) {
                                    // Tàu A vào ray phụ tại Ga A dừng chờ tàu B qua
                                    sA1->setTrackUsed(2);
                                    int travelDuration = arrA - depA; // Thời gian chạy giữa 2 ga
                                    int newDepA = arrB + 5;           // Chờ tàu B đến ga A xong mới xuất phát (+5 phút an toàn)
                                    int delay = newDepA - depA;

                                    sA1->setDepartureMinute(newDepA);
                                    sA2->setArrivalMinute(newDepA + travelDuration);

                                    // Lan truyền trễ cho các ga kế tiếp của tàu A
                                    StopScheduleNode* curr = sA2;
                                    while (curr != nullptr) {
                                        if (curr != sA2) curr->setArrivalMinute(curr->getArrivalMinute() + delay);
                                        curr->setDepartureMinute(curr->getDepartureMinute() + delay);
                                        curr = curr->getNextStop();
                                    }
                                } 
                                else if (!trainA_Waits && stB != nullptr && stB->getNumberOfTracks() > 1) {
                                    // Tàu B vào ray phụ tại Ga B dừng chờ tàu A qua
                                    sB1->setTrackUsed(2);
                                    int travelDuration = arrB - depB;
                                    int newDepB = arrA + 5;
                                    int delay = newDepB - depB;

                                    sB1->setDepartureMinute(newDepB);
                                    sB2->setArrivalMinute(newDepB + travelDuration);

                                    // Lan truyền trễ cho các ga kế tiếp của tàu B
                                    StopScheduleNode* curr = sB2;
                                    while (curr != nullptr) {
                                        if (curr != sB2) curr->setArrivalMinute(curr->getArrivalMinute() + delay);
                                        curr->setDepartureMinute(curr->getDepartureMinute() + delay);
                                        curr = curr->getNextStop();
                                    }
                                }
                            }
                        }
                        sB1 = sB1->getNextStop();
                    }
                    sA1 = sA1->getNextStop();
                }
            }
            trainB = trainB->getNextTrain();
        }
        trainA = trainA->getNextTrain();
    }
}