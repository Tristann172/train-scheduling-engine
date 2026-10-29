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
// Thuật toán kiểm tra và xử lý xung đột cả tại Ga và trên Khu gian ray đơn
void TrainList::resolveConflictAndScheduleSiding(StationList* stationList) {
    for (TrainNode* t1 = head; t1 != nullptr; t1 = t1->getNextTrain()) {
        for (TrainNode* t2 = t1->getNextTrain(); t2 != nullptr; t2 = t2->getNextTrain()) {
            
            StopScheduleNode* s1 = t1->getHeadSchedule();
            while (s1 != nullptr && s1->getNextStop() != nullptr) {
                StopScheduleNode* nextS1 = s1->getNextStop();

                StopScheduleNode* s2 = t2->getHeadSchedule();
                while (s2 != nullptr && s2->getNextStop() != nullptr) {
                    StopScheduleNode* nextS2 = s2->getNextStop();

                    // TRƯỜNG HỢP 1: Xung đột đỗ cùng Ga cùng thời điểm
                    if (s1->getStationID() == s2->getStationID()) {
                        int startOverlap = std::max(s1->getArrivalMinute(), s2->getArrivalMinute());
                        int endOverlap = std::min(s1->getDepartureMinute(), s2->getDepartureMinute());

                        if (startOverlap < endOverlap && s1->getTrackUsed() == 1 && s2->getTrackUsed() == 1) {
                            s2->setTrackUsed(2); // Chuyển sang ray phụ 2
                        }
                    }

                    // TRƯỜNG HỢP 2: Xung đột đối đầu trên Khu gian ray đơn (Ví dụ Vinh - Đồng Hới)
                    // T1 đi từ Ga A -> Ga B, T2 đi từ Ga B -> Ga A
                    if (s1->getStationID() == nextS2->getStationID() && nextS1->getStationID() == s2->getStationID()) {
                        int t1_start = s1->getDepartureMinute();
                        int t1_end = nextS1->getArrivalMinute();

                        int t2_start = s2->getDepartureMinute();
                        int t2_end = nextS2->getArrivalMinute();

                        // Phát hiện khoảng thời gian 2 tàu cùng chạy trên khu gian
                        int trackOverlapStart = std::max(t1_start, t2_start);
                        int trackOverlapEnd = std::min(t1_end, t2_end);

                        if (trackOverlapStart < trackOverlapEnd) {
                            // Tái lập lịch: Bắt Tàu 2 đỗ chờ tại Ga S2 ở Ray phụ 2 cho đến khi Tàu 1 chạy xong khu gian
                            int extraWaitTime = t1_end - t2_start + 10; // Chờ T1 đến nơi + 10 phút an toàn
                            
                            s2->setDepartureMinute(s2->getDepartureMinute() + extraWaitTime);
                            s2->setTrackUsed(2); // Đỗ ray phụ chờ nhường đường
                            
                            // Dời giờ các ga phía sau của Tàu 2
                            t2->applyDelayPropagation(s2->getStationID(), extraWaitTime);
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
