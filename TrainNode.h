#pragma once
#include <string>
#include "CarriageNode.h"
#include "StopScheduleNode.h"

class TrainNode {
private:
    std::string trainCode;
    int direction; // 1: Nam, 0: Bac
    int priority;  // 1: Tau khach nhanh SE, 2: Tau thuong TN, 3: Tau hang
    CarriageNode* headCarriage;
    CarriageNode* tailCarriage;
    StopScheduleNode* headSchedule;
    StopScheduleNode* tailSchedule;
    TrainNode* nextTrain;

public:
    static int inferPriorityFromCode(const std::string& code) {
        if (code.rfind("SE", 0) == 0) return 1; // Ưu tiên cao nhất: SE1, SE2, SE3, SE4...
        if (code.rfind("TN", 0) == 0) return 2; // Ưu tiên trung bình: TN1, TN2...
        return 3;                               // Ưu tiên thấp nhất: Tàu hàng (HH, GS...)
    }

    TrainNode(const std::string& code, int dir, int prio = -1)
        : trainCode(code), direction(dir), 
          priority(prio > 0 ? prio : inferPriorityFromCode(code)),
          headCarriage(nullptr), tailCarriage(nullptr),
          headSchedule(nullptr), tailSchedule(nullptr), nextTrain(nullptr) {}

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
    int getPriority() const { return priority; }
    CarriageNode* getHeadCarriage() const { return headCarriage; }
    StopScheduleNode* getHeadSchedule() const { return headSchedule; }
    StopScheduleNode* getTailSchedule() const { return tailSchedule; }
    TrainNode* getNextTrain() const { return nextTrain; }

    // Setters
    void setPriority(int prio) { priority = prio; }
    void setNextTrain(TrainNode* next) { nextTrain = next; }

    // Chèn toa xe độ phức tạp O(1)
    void addCarriage(int cNo, const std::string& type, int seats) {
        CarriageNode* newNode = new CarriageNode(cNo, type, seats);
        if (!headCarriage) {
            headCarriage = tailCarriage = newNode;
            return;
        }
        tailCarriage->setNextCarriage(newNode);
        tailCarriage = newNode;
    }

    // Chèn lịch dừng độ phức tạp O(1)
    void addSchedule(int stID, int arr, int dep, int track) {
        StopScheduleNode* newNode = new StopScheduleNode(stID, arr, dep, track);
        if (!headSchedule) {
            headSchedule = tailSchedule = newNode;
            return;
        }
        tailSchedule->setNextStop(newNode);
        tailSchedule = newNode;
    }

    // Tìm ga dừng liền trước một ga cụ thể trong hành trình của tàu
    StopScheduleNode* findPreviousStop(StopScheduleNode* targetStop) const {
        if (!headSchedule || headSchedule == targetStop) return nullptr;
        StopScheduleNode* curr = headSchedule;
        while (curr && curr->getNextStop() != targetStop) {
            curr = curr->getNextStop();
        }
        return curr;
    }

    // Lan truyền dời giờ các ga tiếp theo khi xảy ra trễ
    bool applyDelayPropagation(int fromStationID, int delayMinutes, bool delayArrivalAtFirst = true) {
        StopScheduleNode* curr = headSchedule;
        bool found = false;

        while (curr != nullptr) {
            if (curr->getStationID() == fromStationID) {
                found = true;
                if (delayArrivalAtFirst) {
                    curr->setArrivalMinute(curr->getArrivalMinute() + delayMinutes);
                }
                curr->setDepartureMinute(curr->getDepartureMinute() + delayMinutes);
            } else if (found) {
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