#include "TrainList.h"
#include "StationList.h"
#include "StopScheduleNode.h"
#include "StationNode.h"
#include <iostream>
#include <algorithm>

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
    if (!head) {
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
    if (train) {
        train->addCarriage(cNo, type, seats);
    }
}

void TrainList::addStopScheduleToTrain(const string& code, int stID, int arr, int dep, int track) {
    TrainNode* train = findTrainByCode(code);
    if (train) {
        train->addSchedule(stID, arr, dep, track);
    }
}

void TrainList::displayAllTrains() const {
    cout << "\n=== DANH SACH CAC CHUYEN TAU ===\n";
    TrainNode* temp = head;
    while (temp != nullptr) {
        cout << "Ma Tau: " << temp->getTrainCode() 
             << " | Huong Chay: " << (temp->getDirection() == 1 ? "Nam (Di vao)" : "Bac (Di ra)") << "\n";
        temp = temp->getNextTrain();
    }
}

// ==================== THUẬT TOÁN TÁI LẬP LỊCH TRÁNH TÀU (MEET & PASS / SIDING) ====================
void TrainList::resolveConflictAndScheduleSiding(StationList* stationList) {
    for (TrainNode* t1 = head; t1 != nullptr; t1 = t1->getNextTrain()) {
        for (TrainNode* t2 = t1->getNextTrain(); t2 != nullptr; t2 = t2->getNextTrain()) {
            
            StopScheduleNode* s1 = t1->getHeadSchedule();
            while (s1 != nullptr) {
                StopScheduleNode* s2 = t2->getHeadSchedule();
                while (s2 != nullptr) {
                    
                    // Nếu 2 tàu cùng dừng đỗ tại cùng 1 Ga
                    if (s1->getStationID() == s2->getStationID()) {
                        int startOverlap = max(s1->getArrivalMinute(), s2->getArrivalMinute());
                        int endOverlap = min(s1->getDepartureMinute(), s2->getDepartureMinute());

                        // Nếu thời gian đỗ giao thoa trùng nhau và cả 2 cùng đăng ký đỗ tại Ray chính (Ray 1)
                        if (startOverlap < endOverlap && s1->getTrackUsed() == 1 && s2->getTrackUsed() == 1) {
                            // Tái lập lịch: Tự động chuyển Tàu 2 sang đỗ ở Ray 2 (Ray phụ) để tránh đụng độ
                            s2->setTrackUsed(2);
                        }
                    }
                    s2 = s2->getNextStop();
                }
                s1 = s1->getNextStop();
            }
        }
    }
}

// ==================== THUẬT TOÁN DELAY PROPAGATION TRỌN GÓI ====================
bool TrainList::applyDelayAndReschedule(const string& trainCode, int fromStationID, int delayMinutes, StationList* stationList) {
    TrainNode* train = findTrainByCode(trainCode);
    if (!train) return false;

    // BƯỚC 1: Dời giờ đến/đi cho tất cả các ga tiếp theo của tàu bị sự cố
    bool ok = train->applyDelayPropagation(fromStationID, delayMinutes);
    
    // BƯỚC 2: Tự động tái lập lịch tránh tàu ngay sau khi mốc thời gian bị thay đổi
    if (ok) {
        resolveConflictAndScheduleSiding(stationList);
    }
    return ok;
}
