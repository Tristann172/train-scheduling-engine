// ==================== File: TrainList.cpp ====================
#include "TrainList.h"
#include "StationList.h"
#include "StopScheduleNode.h"
#include "StationNode.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>

using namespace std;

static string formatTimeHHMM(int totalMinutes) {
    int h = (totalMinutes / 60) % 24;
    int m = totalMinutes % 60;
    stringstream ss;
    ss << setfill('0') << setw(2) << h << ":" << setfill('0') << setw(2) << m;
    return ss.str();
}

TrainList::TrainList() : head(nullptr), tail(nullptr), logStart(0), logCount(0) {}

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
    tail = nullptr;
}

TrainNode* TrainList::getHead() const { return head; }

void TrainList::addTrain(const string& code, int direction, int priority) {
    TrainNode* newNode = new TrainNode(code, direction, priority);
    if (!head) {
        head = tail = newNode;
        return;
    }
    tail->setNextTrain(newNode);
    tail = newNode;
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
    if (train) train->addCarriage(cNo, type, seats);
}

void TrainList::addStopScheduleToTrain(const string& code, int stID, int arr, int dep, int track) {
    TrainNode* train = findTrainByCode(code);
    if (train) train->addSchedule(stID, arr, dep, track);
}

void TrainList::addConflictLog(const std::string& logMsg) {
    if (logCount < MAX_LOGS) {
        // Chưa đầy mảng: ghi tiếp vào vị trí kế
        int insertIdx = (logStart + logCount) % MAX_LOGS;
        conflictLogs[insertIdx] = logMsg;
        logCount++;
    } else {
        // Đã đầy: ghi đè lên phần tử cũ nhất và dịch vị trí bắt đầu sang phải
        conflictLogs[logStart] = logMsg;
        logStart = (logStart + 1) % MAX_LOGS;
    }
}

std::string TrainList::getLog(int index) const {
    if (index < 0 || index >= logCount) return "";
    int actualIdx = (logStart + index) % MAX_LOGS;
    return conflictLogs[actualIdx];
}

// Tìm ray phụ trống từ ray 2 đến maxTracks tại ga stationID
int TrainList::findAvailableSidingTrack(int stationID, int arr, int dep, int maxTracks, TrainNode* excludeTrain) const {
    if (maxTracks <= 1) return -1;

    const int HEADWAY = 10; // Khoảng đệm an toàn

    for (int candidateTrack = 2; candidateTrack <= maxTracks; ++candidateTrack) {
        bool occupied = false;
        for (TrainNode* tr = head; tr != nullptr; tr = tr->getNextTrain()) {
            if (tr == excludeTrain) continue;

            for (StopScheduleNode* st = tr->getHeadSchedule(); st != nullptr; st = st->getNextStop()) {
                if (st->getStationID() == stationID && st->getTrackUsed() == candidateTrack) {
                    
                    // Dùng std::max và std::min có cộng HEADWAY
                    int overlapStart = std::max(arr, st->getArrivalMinute());
                    int overlapEnd   = std::min(dep + HEADWAY, st->getDepartureMinute() + HEADWAY);

                    if (overlapStart < overlapEnd) {
                        occupied = true;
                        break;
                    }
                }
            }
            if (occupied) break;
        }
        if (!occupied) return candidateTrack;
    }
    return -1;
}

// ==================== THUẬT TOÁN TÁI LẬP LỊCH TRÁNH TÀU ĐA LƯỢT ====================
void TrainList::resolveConflictAndScheduleSiding(StationList* stationList) {
    if (!stationList) return;

    const int SAFETY_HEADWAY = 10; // Khoảng cách an toàn tối thiểu (10 phút)
    const int MAX_PASSES = 10;     // Giới hạn vòng lặp giải tỏa domino
    int passCount = 0;
    bool conflictFound = false;

    do {
        conflictFound = false;
        passCount++;

        for (TrainNode* t1 = head; t1 != nullptr; t1 = t1->getNextTrain()) {
            for (TrainNode* t2 = t1->getNextTrain(); t2 != nullptr; t2 = t2->getNextTrain()) {

                StopScheduleNode* s1 = t1->getHeadSchedule();
                while (s1 != nullptr) {
                    StopScheduleNode* s2 = t2->getHeadSchedule();

                    while (s2 != nullptr) {
                        // =========================================================================
                        // TRƯỜNG HỢP 1: XUNG ĐỘT CÙNG GA CÙNG THỜI ĐIỂM & CÙNG RAY (Station Conflict)
                        // =========================================================================
                        if (s1->getStationID() == s2->getStationID() && s1->getTrackUsed() == s2->getTrackUsed()) {
                            
                            // Sử dụng std::max / std::min có cộng khoảng đệm an toàn
                            int startOverlap = std::max(s1->getArrivalMinute(), s2->getArrivalMinute());
                            int endOverlap   = std::min(s1->getDepartureMinute() + SAFETY_HEADWAY, 
                                                        s2->getDepartureMinute() + SAFETY_HEADWAY);

                            // Phát hiện cấn giờ an toàn tại ga
                            if (startOverlap < endOverlap) {
                                // Phân định tàu nhường: Priority trước, FCFS sau
                                bool t2Yields = true;
                                if (t1->getPriority() != t2->getPriority()) {
                                    t2Yields = (t2->getPriority() > t1->getPriority());
                                } else {
                                    t2Yields = (s2->getArrivalMinute() >= s1->getArrivalMinute());
                                }

                                TrainNode* yieldTrain = t2Yields ? t2 : t1;
                                TrainNode* prioTrain  = t2Yields ? t1 : t2;
                                StopScheduleNode* yieldStop = t2Yields ? s2 : s1;
                                StopScheduleNode* prioStop  = t2Yields ? s1 : s2;

                                StationNode* currStation = stationList->findStationByID(yieldStop->getStationID());
                                int maxTracks = currStation ? currStation->getMaxTracks() : 1;

                                // Dò ray phụ trống tại ga hiện tại
                                int freeSiding = findAvailableSidingTrack(
                                    yieldStop->getStationID(), yieldStop->getArrivalMinute(),
                                    yieldStop->getDepartureMinute(), maxTracks, yieldTrain
                                );

                                if (freeSiding != -1) {
                                    // Ga có ray phụ -> Dạt vào ray phụ an toàn
                                    yieldStop->setTrackUsed(freeSiding);
                                    conflictFound = true;
                                    addConflictLog("[Ga " + std::to_string(yieldStop->getStationID()) + "] " +
                                                   yieldTrain->getTrainCode() + " (P" + std::to_string(yieldTrain->getPriority()) +
                                                   ") vao Ray " + std::to_string(freeSiding) + " nhuong " + prioTrain->getTrainCode());
                                } else {
                                    // Hết ray phụ hoặc ga chỉ có 1 ray -> Đẩy lùi lịch trình về ga trước
                                    StopScheduleNode* prevStop = yieldTrain->findPreviousStop(yieldStop);
                                    int extraWait = std::max(0, prioStop->getDepartureMinute() - yieldStop->getArrivalMinute() + SAFETY_HEADWAY);

                                    if (extraWait > 0) {
                                        if (prevStop != nullptr) {
                                            // Lùi giờ khởi hành tại ga trước
                                            prevStop->setDepartureMinute(prevStop->getDepartureMinute() + extraWait);
                                            
                                            StationNode* prevSt = stationList->findStationByID(prevStop->getStationID());
                                            int prevMaxTracks = prevSt ? prevSt->getMaxTracks() : 1;
                                            
                                            // Kiểm tra ray trống tại ga trước trong khung giờ mới
                                            int prevSiding = findAvailableSidingTrack(
                                                prevStop->getStationID(), prevStop->getArrivalMinute(),
                                                prevStop->getDepartureMinute(), prevMaxTracks, yieldTrain
                                            );
                                            
                                            if (prevSiding != -1) {
                                                prevStop->setTrackUsed(prevSiding); // Có ray phụ -> vào ray phụ
                                            }
                                            // Lan truyền trễ
                                            yieldTrain->applyDelayPropagation(yieldStop->getStationID(), extraWait, true);
                                            addConflictLog("[Ga 1-Ray " + std::to_string(yieldStop->getStationID()) + "] Giu " +
                                                           yieldTrain->getTrainCode() + " tai Ga " + std::to_string(prevStop->getStationID()) +
                                                           " +" + std::to_string(extraWait) + "p nhuong " + prioTrain->getTrainCode());
                                        } else {
                                            // Trường hợp biên: Tàu đang ở ga xuất phát đầu tiên, không thể lùi được nữa
                                            yieldTrain->applyDelayPropagation(yieldStop->getStationID(), extraWait, true);
                                            addConflictLog("[Ga Dau " + std::to_string(yieldStop->getStationID()) + "] Lui khoi hanh " +
                                                           yieldTrain->getTrainCode() + " +" + std::to_string(extraWait) + "p");
                                        }
                                        conflictFound = true; // Kích hoạt Multi-pass lượt kế tiếp
                                    }
                                }
                            }
                        }

                        // =========================================================================
                        // TRƯỜNG HỢP 2: XUNG ĐỘT KHU GIAN ĐƯỜNG ĐƠN (Block Section Conflict)
                        // =========================================================================
                        StopScheduleNode* nextS1 = s1->getNextStop();
                        StopScheduleNode* nextS2 = s2->getNextStop();

                        if (nextS1 != nullptr && nextS2 != nullptr) {
                            bool isHeadOn  = (s1->getStationID() == nextS2->getStationID() && nextS1->getStationID() == s2->getStationID());
                            bool isRearEnd = (s1->getStationID() == s2->getStationID() && nextS1->getStationID() == nextS2->getStationID());

                            if (isHeadOn || isRearEnd) {
                                int t1_start = s1->getDepartureMinute();
                                int t1_end   = nextS1->getArrivalMinute();
                                int t2_start = s2->getDepartureMinute();
                                int t2_end   = nextS2->getArrivalMinute();

                                // Kiểm tra 2 tàu có cùng xuất hiện trên khu gian không
                                int trackOverlapStart = std::max(t1_start, t2_start);
                                int trackOverlapEnd   = std::min(t1_end, t2_end);

                                if (trackOverlapStart < trackOverlapEnd) {
                                    // Phân định tàu nhường
                                    bool t2Yields = true;
                                    if (t1->getPriority() != t2->getPriority()) {
                                        t2Yields = (t2->getPriority() > t1->getPriority());
                                    } else {
                                        t2Yields = (t2_start >= t1_start);
                                    }

                                    TrainNode* yieldTrain = t2Yields ? t2 : t1;
                                    TrainNode* prioTrain  = t2Yields ? t1 : t2;
                                    StopScheduleNode* yieldStop = t2Yields ? s2 : s1;
                                    StopScheduleNode* yieldNext = t2Yields ? nextS2 : nextS1;
                                    
                                    int prioEnd  = t2Yields ? t1_end : t2_end;
                                    int yieldDep = t2Yields ? t2_start : t1_start;

                                    int extraWaitTime = std::max(0, prioEnd - yieldDep + SAFETY_HEADWAY);

                                    if (extraWaitTime > 0) {
                                        // Cộng giờ chờ tại ga trước khi vào khu gian
                                        yieldStop->setDepartureMinute(yieldStop->getDepartureMinute() + extraWaitTime);

                                        StationNode* waitSt = stationList->findStationByID(yieldStop->getStationID());
                                        int maxTracks = waitSt ? waitSt->getMaxTracks() : 1;
                                        
                                        // Tìm ray phụ trong thời gian chờ
                                        int freeSiding = findAvailableSidingTrack(
                                            yieldStop->getStationID(), yieldStop->getArrivalMinute(),
                                            yieldStop->getDepartureMinute(), maxTracks, yieldTrain
                                        );
                                        
                                        if (freeSiding != -1) yieldStop->setTrackUsed(freeSiding);

                                        // Lan truyền trễ
                                        yieldTrain->applyDelayPropagation(yieldNext->getStationID(), extraWaitTime, true);

                                        conflictFound = true;
                                        addConflictLog("[Khu gian " + std::to_string(yieldStop->getStationID()) + "->" +
                                                       std::to_string(yieldNext->getStationID()) + "] " + yieldTrain->getTrainCode() +
                                                       " (P" + std::to_string(yieldTrain->getPriority()) + ") cho " +
                                                       prioTrain->getTrainCode() + " +" + std::to_string(extraWaitTime) +
                                                       "p (Xuat phat: " + formatTimeHHMM(yieldStop->getDepartureMinute()) + ")");
                                    }
                                }
                            }
                        }

                        s2 = s2->getNextStop();
                    }
                    s1 = s1->getNextStop();
                }
            }
        }
    } while (conflictFound && passCount < MAX_PASSES);
}

// ==================== THUẬT TOÁN DELAY PROPAGATION TRỌN GÓI ====================
bool TrainList::applyDelayAndReschedule(const string& trainCode, int fromStationID, int delayMinutes, StationList* stationList) {
    TrainNode* train = findTrainByCode(trainCode);
    if (!train) return false;

    addConflictLog("[Su co] Tau " + trainCode + " bi tre +" + to_string(delayMinutes) + "p tu Ga " + to_string(fromStationID));
    bool ok = train->applyDelayPropagation(fromStationID, delayMinutes, true);

    if (ok) {
        resolveConflictAndScheduleSiding(stationList);
    }
    return ok;
}