#include "TrainList.h" 
#include "StationList.h"
#include "StopScheduleNode.h"
#include "StationNode.h"
#include "MinHeap.h"
#include "HashMap.h"
#include <iomanip>
#include <sstream>

using namespace std;

namespace {
    string formatTimeHHMM(int totalMinutes) {
        int hour = (totalMinutes / 60) % 24;
        int minute = totalMinutes % 60;
        stringstream stream;
        stream << setfill('0') << setw(2) << hour << ":"
               << setfill('0') << setw(2) << minute;
        return stream.str();
    }

    struct ScheduleEvent {
        int time;
        int endTime;
        int type; // 0: ga/ray, 1: khu gian
        int stationID;
        int targetStationID;
        int track;
        int sequence;
        unsigned long trainRevision;
        TrainNode* train;
        StopScheduleNode* stop;
        StopScheduleNode* nextStop;
        string trainCode;

        ScheduleEvent()
            : time(0), endTime(0), type(0), stationID(0), targetStationID(0),
              track(0), sequence(0), trainRevision(0), train(nullptr),
              stop(nullptr), nextStop(nullptr) {}
    };

    bool isCurrentEvent(const ScheduleEvent& event) {
        return event.train &&
               event.trainRevision == event.train->getScheduleRevision();
    }

    struct ScheduleEventBefore {
        bool operator()(const ScheduleEvent& left, const ScheduleEvent& right) const {
            if (left.time != right.time) return left.time < right.time;
            if (left.train->getPriority() != right.train->getPriority()) {
                return left.train->getPriority() < right.train->getPriority();
            }
            return left.sequence < right.sequence;
        }
    };

    void pushTrainEvents(MinHeap<ScheduleEvent, ScheduleEventBefore>& events,
                         TrainNode* train, int& sequence, int safetyHeadway) {
        LinkedList<StopScheduleNode*>::Node* stopNode = train->getHeadSchedule();
        while (stopNode) {
            StopScheduleNode* stop = stopNode->data;
            ScheduleEvent stationEvent;
            stationEvent.time = stop->getArrivalMinute();
            stationEvent.endTime = stop->getDepartureMinute() + safetyHeadway;
            stationEvent.type = 0;
            stationEvent.stationID = stop->getStationID();
            stationEvent.targetStationID = stop->getStationID();
            stationEvent.track = stop->getTrackUsed();
            stationEvent.sequence = sequence++;
            stationEvent.trainRevision = train->getScheduleRevision();
            stationEvent.train = train;
            stationEvent.stop = stop;
            stationEvent.trainCode = train->getTrainCode();
            if (stationEvent.endTime > stationEvent.time) {
                events.push(stationEvent);
            }

            if (stopNode->next) {
                StopScheduleNode* nextStop = stopNode->next->data;
                ScheduleEvent blockEvent;
                blockEvent.time = stop->getDepartureMinute();
                blockEvent.endTime = nextStop->getArrivalMinute() + safetyHeadway;
                blockEvent.type = 1;
                blockEvent.stationID = stop->getStationID();
                blockEvent.targetStationID = nextStop->getStationID();
                blockEvent.sequence = sequence++;
                blockEvent.trainRevision = train->getScheduleRevision();
                blockEvent.train = train;
                blockEvent.stop = stop;
                blockEvent.nextStop = nextStop;
                blockEvent.trainCode = train->getTrainCode();
                if (blockEvent.endTime > blockEvent.time) {
                    events.push(blockEvent);
                }
            }
            stopNode = stopNode->next;
        }
    }

    // KHÓA TÀI NGUYÊN (Sử dụng cho HashMap của bạn)
    struct ResourceKey {
        int type; // 0: Ray trong ga, 1: Khu gian
        int id1;  
        int id2;  

        ResourceKey(int t = 0, int a = 0, int b = 0) {
            type = t;
            if (type == 1 && a > b) {
                id1 = b;
                id2 = a;
            } else {
                id1 = a;
                id2 = b;
            }
        }

        // Toán tử == để HashMap kiểm tra trùng lặp khóa
        bool operator==(const ResourceKey& other) const {
            return type == other.type && id1 == other.id1 && id2 == other.id2;
        }
    };

    // HÀM BĂM (Functor) DÀNH CHO KHÓA TÀI NGUYÊN
    struct ResourceKeyHash {
        unsigned long operator()(const ResourceKey& k) const {
            unsigned long hash = 2166136261UL;
            hash = (hash ^ static_cast<unsigned long>(k.type + 1)) * 16777619UL;
            hash = (hash ^ static_cast<unsigned long>(k.id1)) * 16777619UL;
            hash = (hash ^ static_cast<unsigned long>(k.id2)) * 16777619UL;
            return hash;
        }
    };

    // Hàm phụ trợ trích xuất Key từ sự kiện
    ResourceKey getEventKey(const ScheduleEvent& ev) {
        if (ev.type == 0) {
            return ResourceKey(0, ev.stationID, ev.track);
        }
        return ResourceKey(1, ev.stationID, ev.targetStationID);
    }
}

TrainList::TrainList()
    : trains(), trainsByCode(), structureRevision(0), logStart(0), logCount(0) {}

TrainList::~TrainList() {
    clear();
}

void TrainList::clear() {
    LinkedList<TrainNode*>::Node* train = trains.getHead();
    while (train) {
        delete train->data;
        train = train->next;
    }
    trains.clear();
    trainsByCode.clear();
    ++structureRevision;
}

LinkedList<TrainNode*>::Node* TrainList::getHead() const {
    return trains.getHead();
}

void TrainList::addTrain(const string& code, int direction, int priority) {
    TrainNode* newNode = new TrainNode(code, direction, priority);
    trains.pushBack(newNode);
    if (!trainsByCode.find(code)) {
        trainsByCode.insert(code, newNode);
    }
    ++structureRevision;
}

TrainNode* TrainList::findTrainByCode(const string& code) const {
    TrainNode* const* train = trainsByCode.find(code);
    return train ? *train : nullptr;
}

void TrainList::addCarriageToTrain(const string& code, int carriageNo,
                                   const string& type, int seats) {
    TrainNode* train = findTrainByCode(code);
    if (train) {
        train->addCarriage(carriageNo, type, seats);
        ++structureRevision;
    }
}

void TrainList::addStopScheduleToTrain(const string& code, int stationID,
                                       int arrival, int departure, int track) {
    TrainNode* train = findTrainByCode(code);
    if (train) {
        train->addSchedule(stationID, arrival, departure, track);
        ++structureRevision;
    }
}

void TrainList::addConflictLog(const string& logMessage) {
    if (logCount < MAX_LOGS) {
        const int insertIndex = (logStart + logCount) % MAX_LOGS;
        conflictLogs[insertIndex] = logMessage;
        ++logCount;
    } else {
        conflictLogs[logStart] = logMessage;
        logStart = (logStart + 1) % MAX_LOGS;
    }
}

string TrainList::getLog(int index) const {
    if (index < 0 || index >= logCount) return "";
    return conflictLogs[(logStart + index) % MAX_LOGS];
}

int TrainList::findAvailableSidingTrack(int stationID, int arrival, int departure,
                                        int maxTracks, TrainNode* excludeTrain) const {
    if (maxTracks <= 1) return -1;

    const int HEADWAY = 10;
    for (int candidateTrack = 2; candidateTrack <= maxTracks; ++candidateTrack) {
        bool occupied = false;
        for (LinkedList<TrainNode*>::Node* trainNode = trains.getHead();
             trainNode && !occupied; trainNode = trainNode->next) {
            TrainNode* train = trainNode->data;
            if (train == excludeTrain) continue;

            for (LinkedList<StopScheduleNode*>::Node* stopNode =
                     train->getHeadSchedule(); stopNode; stopNode = stopNode->next) {
                StopScheduleNode* stop = stopNode->data;
                if (stop->getStationID() != stationID ||
                    stop->getTrackUsed() != candidateTrack) continue;

                const int overlapStart = arrival > stop->getArrivalMinute()
                    ? arrival : stop->getArrivalMinute();
                const int stopEnd = stop->getDepartureMinute() + HEADWAY;
                const int requestedEnd = departure + HEADWAY;
                const int overlapEnd = requestedEnd < stopEnd
                    ? requestedEnd : stopEnd;
                if (overlapStart < overlapEnd) {
                    occupied = true;
                    break;
                }
            }
        }
        if (!occupied) return candidateTrack;
    }
    return -1;
}

void TrainList::resolveConflictAndScheduleSiding(StationList* stationList) {
    if (!stationList) return;

    const int SAFETY_HEADWAY = 10;
    MinHeap<ScheduleEvent, ScheduleEventBefore> events;
    int sequence = 0;
    for (LinkedList<TrainNode*>::Node* trainNode = trains.getHead();
         trainNode; trainNode = trainNode->next) {
        TrainNode* train = trainNode->data;
        pushTrainEvents(events, train, sequence, SAFETY_HEADWAY);
    }

    HashMap<ResourceKey, ScheduleEvent, ResourceKeyHash> activeResources(32);

    while (!events.isEmpty()) {
        const ScheduleEvent current = events.top();
        events.pop();
        if (!isCurrentEvent(current)) {
            continue;
        }

        ResourceKey resKey = getEventKey(current);
        ScheduleEvent* occupiedEvent = activeResources.find(resKey);

        // 1. Giải phóng tài nguyên nếu tàu cũ đã rời đi hoặc đổi lịch
        if (occupiedEvent && (!isCurrentEvent(*occupiedEvent) || occupiedEvent->endTime <= current.time)) {
            activeResources.remove(resKey);
            occupiedEvent = nullptr;
        }

        // 2. Nếu tài nguyên trống, tàu hiện tại chiếm dụng
        if (!occupiedEvent) {
            activeResources.insert(resKey, current);
            continue;
        }

        // 3. Cùng một tàu kéo dài thời gian đỗ
        if (occupiedEvent->train == current.train) {
            if (current.endTime > occupiedEvent->endTime) {
                occupiedEvent->endTime = current.endTime;
            }
            continue;
        }

        // 4. Phát hiện xung đột, kiểm tra ưu tiên nhường đường
        const ScheduleEvent other = *occupiedEvent;
        const bool currentYields =
            current.train->getPriority() != other.train->getPriority()
                ? current.train->getPriority() > other.train->getPriority()
                : (current.time != other.time
                       ? current.time > other.time
                       : current.sequence > other.sequence);
        
        const ScheduleEvent& yielding = currentYields ? current : other;
        const ScheduleEvent& priority = currentYields ? other : current;
        TrainNode* yieldingTrain = yielding.train;
        TrainNode* priorityTrain = priority.train;
        bool changed = false;

        if (yielding.type == 0) {
            StationNode* station = stationList->findStationByID(yielding.stationID);
            const int trackCount = station ? station->getMaxTracks() : 1;
            const int siding = findAvailableSidingTrack(
                yielding.stationID, yielding.stop->getArrivalMinute(),
                yielding.stop->getDepartureMinute(), trackCount, yieldingTrain);

            if (siding != -1) {
                yielding.stop->setTrackUsed(siding);
                const int wait = priority.endTime - yielding.time;
                if (wait > 0 && priority.type == 1) { // Nếu nhường tàu đang qua khu gian
                    yielding.stop->setDepartureMinute(yielding.stop->getDepartureMinute() + wait);
                    yieldingTrain->applyDelayPropagation(yielding.stationID, wait, false);
                }
                addConflictLog("[Ga " + to_string(yielding.stationID) + "] " +
                            yieldingTrain->getTrainCode() + " vao Ray " + to_string(siding));
                changed = true;
            } else {
                const int wait = priority.endTime - yielding.time;
                if (wait > 0) {
                    StopScheduleNode* previous = yieldingTrain->findPreviousStop(yielding.stop);
                    if (previous) {
                        previous->setDepartureMinute(previous->getDepartureMinute() + wait);
                    }
                    yieldingTrain->applyDelayPropagation(yielding.stationID, wait, true);
                    changed = true;
                }
            }
        } else { // type == 1 (Khu gian)
            int wait = priority.endTime - yielding.time;
            
            // Xử lý Headway nếu 2 tàu chạy cùng chiều vào cùng ga đích
            if (yielding.targetStationID == priority.targetStationID) {
                wait = (priority.stop->getDepartureMinute() + 
                    (priority.nextStop->getArrivalMinute() - priority.stop->getDepartureMinute()) + 
                    SAFETY_HEADWAY) - yielding.time;
            }

            if (wait > 0) {
                yielding.stop->setDepartureMinute(yielding.stop->getDepartureMinute() + wait);
                
                StationNode* waitStation = stationList->findStationByID(yielding.stationID);
                const int waitTracks = waitStation ? waitStation->getMaxTracks() : 1;
                const int siding = findAvailableSidingTrack(
                    yielding.stationID, yielding.stop->getArrivalMinute(),
                    yielding.stop->getDepartureMinute(), waitTracks, yieldingTrain);
                    
                if (siding != -1) {
                    yielding.stop->setTrackUsed(siding);
                }
                
                yieldingTrain->applyDelayPropagation(yielding.targetStationID, wait, true);
                addConflictLog("[Khu gian " + to_string(yielding.stationID) + "->" +
                            to_string(yielding.targetStationID) + "] " +
                            yieldingTrain->getTrainCode() + " cho " +
                            priorityTrain->getTrainCode() + " +" + to_string(wait) + "p");
                changed = true;
            }
        }

        if (changed) {
            // Thay thế tàu bị loại ra khỏi tài nguyên nếu cần
            if (!currentYields) {
                activeResources.insert(resKey, current);
            }
            yieldingTrain->bumpScheduleRevision();
            pushTrainEvents(events, yieldingTrain, sequence, SAFETY_HEADWAY);
        }
    }
}

bool TrainList::applyDelayAndReschedule(const string& trainCode, int fromStationID,
                                        int delayMinutes, StationList* stationList) {
    TrainNode* train = findTrainByCode(trainCode);
    if (!train) return false;

    addConflictLog("[Su co] Tau " + trainCode + " bi tre +" +
                   to_string(delayMinutes) + "p tu Ga " +
                   to_string(fromStationID));
    const bool delayed =
        train->applyDelayPropagation(fromStationID, delayMinutes, true);
    if (delayed) {
        resolveConflictAndScheduleSiding(stationList);
    }
    return delayed;
}