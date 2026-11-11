#pragma once
#include <string>
#include "CarriageNode.h"
#include "StopScheduleNode.h"
#include "LinkedList.h"

struct Train {
    std::string trainCode;
    int direction; // 1: Nam, 0: Bac
    int priority;  // 1: Tau khach nhanh SE, 2: Tau thuong TN, 3: Tau hang
    LinkedList<Carriage*> carriages;
    LinkedList<StopSchedule*> schedule;
    unsigned long scheduleRevision;

public:
    static int inferPriorityFromCode(const std::string& code) {
        if (code.rfind("SE", 0) == 0) return 1; // Ưu tiên cao nhất: SE1, SE2, SE3, SE4...
        if (code.rfind("TN", 0) == 0) return 2; // Ưu tiên trung bình: TN1, TN2...
        return 3;                               // Ưu tiên thấp nhất: Tàu hàng (HH, GS...)
    }

    Train(const std::string& code, int dir, int prio = -1)
        : trainCode(code), direction(dir), 
          priority(prio > 0 ? prio : inferPriorityFromCode(code)),
          scheduleRevision(0) {}

    ~Train() {
        LinkedList<Carriage*>::Node* carriage = carriages.getHead();
        while (carriage) {
            delete carriage->data;
            carriage = carriage->next;
        }
        LinkedList<StopSchedule*>::Node* stop = schedule.getHead();
        while (stop) {
            delete stop->data;
            stop = stop->next;
        }
    }

    // Getters
    std::string getTrainCode() const { return trainCode; }
    int getDirection() const { return direction; }
    int getPriority() const { return priority; }
    LinkedList<Carriage*>::Node* getHeadCarriage() const { return carriages.getHead(); }
    LinkedList<StopSchedule*>::Node* getHeadSchedule() const { return schedule.getHead(); }
    LinkedList<StopSchedule*>::Node* getTailSchedule() const { return schedule.getTail(); }
    unsigned long getScheduleRevision() const { return scheduleRevision; }

    // Setters
    void setPriority(int prio) { priority = prio; }
    void setDirection(int dir) { direction = dir; }
    void bumpScheduleRevision() { ++scheduleRevision; }

    // Chèn toa xe độ phức tạp O(1)
    CarriageNode* addCarriage(int cNo, const std::string& type) {
        if (cNo <= 0 || type.empty() || findCarriage(cNo)) return nullptr;
        CarriageNode* carriage = new CarriageNode(cNo, type);
        carriages.pushBack(carriage);
        return carriage;
    }

    // Chèn lịch dừng độ phức tạp O(1)
    StopScheduleNode* addSchedule(int stID, int arr, int dep, int track) {
        if (stID <= 0 || arr < 0 || dep < arr || track < 1) return nullptr;
        for (LinkedList<StopSchedule*>::Node* stop = schedule.getHead(); stop; stop = stop->next) {
            if (stop->data->getStationID() == stID) return nullptr;
        }
        StopScheduleNode* newStop = new StopScheduleNode(stID, arr, dep, track);
        schedule.pushBack(newStop);
        return newStop;
    }

    // Tìm ga dừng liền trước một ga cụ thể trong hành trình của tàu
    StopScheduleNode* findPreviousStop(StopScheduleNode* targetStop) const {
        LinkedList<StopSchedule*>::Node* curr = schedule.getHead();
        if (!curr || curr->data == targetStop) return nullptr;
        while (curr && (!curr->next || curr->next->data != targetStop)) {
            curr = curr->next;
        }
        return curr ? curr->data : nullptr;
    }

    // Lan truyền dời giờ các ga tiếp theo khi xảy ra trễ
    bool applyDelayPropagation(int fromStationID, int delayMinutes, bool delayArrivalAtFirst = true) {
        LinkedList<StopSchedule*>::Node* curr = schedule.getHead();
        bool found = false;

        while (curr != nullptr) {
            StopScheduleNode* stop = curr->data;
            if (stop->getStationID() == fromStationID) {
                found = true;
                if (delayArrivalAtFirst) {
                    stop->setArrivalMinute(stop->getArrivalMinute() + delayMinutes);
                }
                stop->setDepartureMinute(stop->getDepartureMinute() + delayMinutes);
            } else if (found) {
                stop->setArrivalMinute(stop->getArrivalMinute() + delayMinutes);
                stop->setDepartureMinute(stop->getDepartureMinute() + delayMinutes);
            }
            curr = curr->next;
        }
        return found;
    }

    CarriageNode* findCarriage(int carriageNo) const {
        LinkedList<Carriage*>::Node* current = carriages.getHead();
        while (current) {
            CarriageNode* carriage = current->data;
            if (carriage->getCarriageNo() == carriageNo) return carriage;
            current = current->next;
        }
        return nullptr;
    }
};

typedef Train TrainNode;