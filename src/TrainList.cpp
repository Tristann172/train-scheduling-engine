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

    // Thêm tham số currentTime = 0 vào cuối
    void pushTrainEvents(MinHeap<ScheduleEvent, ScheduleEventBefore>& events,
                        TrainNode* train, int& sequence, int safetyHeadway, int currentTime = 0) {
        LinkedList<StopScheduleNode*>::Node* stopNode = train->getHeadSchedule();
        while (stopNode) {
            StopScheduleNode* stop = stopNode->data;
            
            // --- TẠO SỰ KIỆN TẠI GA ---
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
            
            // Chỉ nạp nếu sự kiện kết thúc ở TƯƠNG LAI so với currentTime
            if (stationEvent.endTime > stationEvent.time && stationEvent.endTime > currentTime) {
                events.push(stationEvent);
            }

            // --- TẠO SỰ KIỆN KHU GIAN ---
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
                
                // Chỉ nạp nếu sự kiện kết thúc ở TƯƠNG LAI so với currentTime
                if (blockEvent.endTime > blockEvent.time && blockEvent.endTime > currentTime) {
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
    : trains(), trainsByCode(), structureRevision(0), logStart(0), logCount(0),
      maintenanceWindows(), maintenanceCount(0) {}

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
    maintenanceCount = 0;
    ++structureRevision;
}

LinkedList<TrainNode*>::Node* TrainList::getHead() const {
    return trains.getHead();
}

bool TrainList::addTrain(const string& code, int direction, int priority) {
    if (code.empty() || (direction != 0 && direction != 1) ||
        priority > 3 ||
        trainsByCode.find(code)) return false;
    TrainNode* newNode = new TrainNode(code, direction, priority);
    trains.pushBack(newNode);
    trainsByCode.insert(code, newNode);
    ++structureRevision;
    return true;
}

TrainNode* TrainList::findTrainByCode(const string& code) const {
    TrainNode* const* train = trainsByCode.find(code);
    return train ? *train : nullptr;
}

bool TrainList::updateTrain(const string& code, int direction, int priority,
                            const StationList* stationList) {
    TrainNode* train = findTrainByCode(code);
    if (!train || !stationList || (direction != 0 && direction != 1) || priority > 3) {
        return false;
    }
    // Chỉ đổi hướng khi thứ tự các ga hiện tại vẫn hợp lệ theo mốc km.
    StopScheduleNode* previous = nullptr;
    for (LinkedList<StopScheduleNode*>::Node* stopNode = train->getHeadSchedule();
         stopNode; stopNode = stopNode->next) {
        StopScheduleNode* current = stopNode->data;
        StationNode* currentStation = stationList->findStationByID(current->getStationID());
        StationNode* previousStation = previous
            ? stationList->findStationByID(previous->getStationID()) : nullptr;
        if (!currentStation || (previous &&
            (!previousStation ||
             (direction == 1
                ? currentStation->getKmMarker() <= previousStation->getKmMarker()
                : currentStation->getKmMarker() >= previousStation->getKmMarker())))) {
            return false;
        }
        previous = current;
    }
    train->setDirection(direction);
    train->setPriority(priority > 0 ? priority : TrainNode::inferPriorityFromCode(code));
    train->bumpScheduleRevision();
    ++structureRevision;
    return true;
}

bool TrainList::removeTrain(const string& code) {
    TrainNode* train = findTrainByCode(code);
    if (!train) return false;
    LinkedList<TrainNode*>::Node* node = trains.getHead();
    while (node && node->data != train) node = node->next;
    if (!node || !trains.removeNode(node)) return false;
    trainsByCode.remove(code);
    delete train;
    ++structureRevision;
    return true;
}

bool TrainList::addCarriageToTrain(const string& code, int carriageNo,
                                   const string& type) {
    TrainNode* train = findTrainByCode(code);
    if (!train || !train->addCarriage(carriageNo, type)) return false;
    ++structureRevision;
    return true;
}

bool TrainList::updateCarriage(const string& code, int carriageNo,
                               const string& type) {
    TrainNode* train = findTrainByCode(code);
    CarriageNode* carriage = train ? train->findCarriage(carriageNo) : nullptr;
    if (!carriage || type.empty()) return false;
    carriage->setType(type);
    ++structureRevision;
    return true;
}

bool TrainList::removeCarriage(const string& code, int carriageNo) {
    TrainNode* train = findTrainByCode(code);
    if (!train) return false;
    LinkedList<CarriageNode*>::Node* node = train->getHeadCarriage();
    while (node && node->data->getCarriageNo() != carriageNo) node = node->next;
    if (!node) return false;
    delete node->data;
    train->carriages.removeNode(node);
    ++structureRevision;
    return true;
}

bool TrainList::addStopScheduleToTrain(const string& code, int stationID,
                                       int arrival, int departure, int track,
                                       StationList* stationList) {
    TrainNode* train = findTrainByCode(code);
    StationNode* station = stationList ? stationList->findStationByID(stationID) : nullptr;
    LinkedList<StopScheduleNode*>::Node* tailNode =
        train ? train->getTailSchedule() : nullptr;
    StopScheduleNode* tail = tailNode ? tailNode->data : nullptr;
    StationNode* previousStation = tail && stationList
        ? stationList->findStationByID(tail->getStationID()) : nullptr;
    if (!train || !station || track > station->getMaxTracks() ||
        (tail && (arrival < tail->getDepartureMinute() || !previousStation ||
            (train->getDirection() == 1
                ? station->getKmMarker() <= previousStation->getKmMarker()
                : station->getKmMarker() >= previousStation->getKmMarker())))) return false;
    if (!train->addSchedule(stationID, arrival, departure, track)) return false;
    train->bumpScheduleRevision();
    ++structureRevision;
    // Kiểm tra và tái điều phối ngay để mọi nơi gọi API đều được xử lý xung đột.
    resolveConflictAndScheduleSiding(stationList);
    return true;
}

bool TrainList::updateStopSchedule(const string& code, int stationID, int arrival,
                                   int departure, int track,
                                   const StationList* stationList) {
    TrainNode* train = findTrainByCode(code);
    if (!train || arrival < 0 || departure < arrival || track < 1) return false;
    LinkedList<StopScheduleNode*>::Node* target = train->getHeadSchedule();
    while (target && target->data->getStationID() != stationID) target = target->next;
    StationNode* station = stationList ? stationList->findStationByID(stationID) : nullptr;
    if (!target || !station || track > station->getMaxTracks()) return false;

    StopScheduleNode* previous = train->findPreviousStop(target->data);
    StopScheduleNode* next = target->next ? target->next->data : nullptr;
    StationNode* previousStation = previous && stationList
        ? stationList->findStationByID(previous->getStationID()) : nullptr;
    StationNode* nextStation = next && stationList
        ? stationList->findStationByID(next->getStationID()) : nullptr;
    if ((previous && (arrival < previous->getDepartureMinute() || !previousStation ||
            (train->getDirection() == 1
                ? station->getKmMarker() <= previousStation->getKmMarker()
                : station->getKmMarker() >= previousStation->getKmMarker()))) ||
        (next && (departure > next->getArrivalMinute() || !nextStation ||
            (train->getDirection() == 1
                ? station->getKmMarker() >= nextStation->getKmMarker()
                : station->getKmMarker() <= nextStation->getKmMarker())))) return false;

    target->data->setArrivalMinute(arrival);
    target->data->setDepartureMinute(departure);
    target->data->setTrackUsed(track);
    train->bumpScheduleRevision();
    ++structureRevision;
    return true;
}

bool TrainList::removeStopSchedule(const string& code, int stationID) {
    TrainNode* train = findTrainByCode(code);
    if (!train || train->schedule.getSize() <= 2) return false;
    LinkedList<StopScheduleNode*>::Node* target = train->getHeadSchedule();
    while (target && target->data->getStationID() != stationID) target = target->next;
    if (!target) return false;
    delete target->data;
    train->schedule.removeNode(target);
    train->bumpScheduleRevision();
    ++structureRevision;
    return true;
}

bool TrainList::hasStationStop(int stationID) const {
    for (LinkedList<TrainNode*>::Node* trainNode = trains.getHead(); trainNode; trainNode = trainNode->next) {
        for (LinkedList<StopScheduleNode*>::Node* stop = trainNode->data->getHeadSchedule(); stop; stop = stop->next) {
            if (stop->data->getStationID() == stationID) return true;
        }
    }
    return false;
}

bool TrainList::hasTrackAbove(int stationID, int track) const {
    for (LinkedList<TrainNode*>::Node* trainNode = trains.getHead(); trainNode; trainNode = trainNode->next) {
        for (LinkedList<StopScheduleNode*>::Node* stop = trainNode->data->getHeadSchedule(); stop; stop = stop->next) {
            if (stop->data->getStationID() == stationID && stop->data->getTrackUsed() > track) return true;
        }
    }
    return false;
}

bool TrainList::hasMaintenanceForStation(int stationID) const {
    for (int i = 0; i < maintenanceCount; ++i) {
        const MaintenanceWindow& window = maintenanceWindows[i];
        if ((window.resourceType == 0 || window.resourceType == 2) &&
            window.firstID == stationID) return true;
        if (window.resourceType == 1 &&
            (window.firstID == stationID || window.secondID == stationID)) return true;
    }
    return false;
}

bool TrainList::hasMaintenanceBlockForStation(int stationID) const {
    for (int i = 0; i < maintenanceCount; ++i) {
        const MaintenanceWindow& window = maintenanceWindows[i];
        if (window.resourceType == 1 &&
            (window.firstID == stationID || window.secondID == stationID)) return true;
    }
    return false;
}

bool TrainList::hasMaintenanceTrackAbove(int stationID, int track) const {
    for (int i = 0; i < maintenanceCount; ++i) {
        const MaintenanceWindow& window = maintenanceWindows[i];
        if (window.resourceType == 2 && window.firstID == stationID &&
            window.secondID > track) return true;
    }
    return false;
}

bool TrainList::addMaintenance(const string& resource, int firstID, int secondID,
                               int startMinute, int endMinute) {
    int type = -1;
    if (resource == "STATION") type = 0;
    else if (resource == "BLOCK") type = 1;
    else if (resource == "TRACK") type = 2;
    if (type < 0 || firstID <= 0 || startMinute < 0 ||
        endMinute <= startMinute ||
        (type == 2 && secondID < 1) ||
        (type == 1 && (secondID <= 0 || firstID == secondID))) return false;
    if (type == 1 && firstID > secondID) {
        const int temp = firstID;
        firstID = secondID;
        secondID = temp;
    }
    for (int i = 0; i < maintenanceCount; ++i) {
        const MaintenanceWindow& existing = maintenanceWindows[i];
        if (existing.resourceType == type && existing.firstID == firstID &&
            existing.secondID == secondID && existing.startMinute == startMinute &&
            existing.endMinute == endMinute) return true;
    }
    if (maintenanceCount >= MAX_MAINTENANCE_WINDOWS) return false;
    maintenanceWindows[maintenanceCount++] =
        {type, firstID, secondID, startMinute, endMinute};
    return true;
}

bool TrainList::removeMaintenance(const string& resource, int firstID, int secondID,
                                 int startMinute, int endMinute) {
    int type = -1;
    if (resource == "STATION") type = 0;
    else if (resource == "BLOCK") type = 1;
    else if (resource == "TRACK") type = 2;
    if (type < 0) return false;
    if (type == 1 && firstID > secondID) {
        const int temp = firstID;
        firstID = secondID;
        secondID = temp;
    }
    for (int i = 0; i < maintenanceCount; ++i) {
        const MaintenanceWindow& existing = maintenanceWindows[i];
        if (existing.resourceType == type && existing.firstID == firstID &&
            existing.secondID == secondID && existing.startMinute == startMinute &&
            existing.endMinute == endMinute) {
            for (int j = i + 1; j < maintenanceCount; ++j) {
                maintenanceWindows[j - 1] = maintenanceWindows[j];
            }
            --maintenanceCount;
            return true;
        }
    }
    return false;
}

bool TrainList::isUnderMaintenanceAt(const string& resource, int firstID,
                                     int secondID, int minute) const {
    int type = -1;
    if (resource == "STATION") type = 0;
    else if (resource == "BLOCK") type = 1;
    else if (resource == "TRACK") type = 2;
    if (type < 0) return false;
    if (type == 1 && firstID > secondID) {
        const int temp = firstID;
        firstID = secondID;
        secondID = temp;
    }
    for (int i = 0; i < maintenanceCount; ++i) {
        const MaintenanceWindow& repair = maintenanceWindows[i];
        if (repair.resourceType == type && repair.firstID == firstID &&
            repair.secondID == secondID && minute >= repair.startMinute &&
            minute < repair.endMinute) return true;
    }
    return false;
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
        const int requestedEnd = departure + HEADWAY;
        for (int i = 0; i < maintenanceCount; ++i) {
            const MaintenanceWindow& repair = maintenanceWindows[i];
            if (repair.resourceType == 0 && repair.firstID == stationID &&
                arrival < repair.endMinute && repair.startMinute < requestedEnd) {
                occupied = true;
                break;
            }
            if (repair.resourceType == 2 && repair.firstID == stationID &&
                repair.secondID == candidateTrack &&
                arrival < repair.endMinute && repair.startMinute < requestedEnd) {
                occupied = true;
                break;
            }
        }
        if (occupied) continue;
        for (LinkedList<TrainNode*>::Node* trainNode = trains.getHead();
             trainNode && !occupied; trainNode = trainNode->next) {
            TrainNode* train = trainNode->data;
            if (train == excludeTrain) continue;

            for (LinkedList<StopScheduleNode*>::Node* stopNode =
                     train->getHeadSchedule(); stopNode; stopNode = stopNode->next) {
                StopScheduleNode* stop = stopNode->data;
                if (stop->getStationID() != stationID ||
                    stop->getTrackUsed() != candidateTrack) continue;
                // Tàu kết thúc hành trình được xem là chiếm chỗ đỗ tại ga cuối.
                if (!stopNode->next) {
                    occupied = true;
                    break;
                }

                const int overlapStart = arrival > stop->getArrivalMinute()
                    ? arrival : stop->getArrivalMinute();
                const int stopEnd = stop->getDepartureMinute() + HEADWAY;
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
    for (LinkedList<TrainNode*>::Node* trainNode = trains.getHead();
         trainNode; trainNode = trainNode->next) {
        TrainNode* train = trainNode->data;
        LinkedList<StopScheduleNode*>::Node* lastStopNode = train->getTailSchedule();
        StopScheduleNode* lastStop = lastStopNode ? lastStopNode->data : nullptr;
        StationNode* station = lastStop
            ? stationList->findStationByID(lastStop->getStationID()) : nullptr;
        if (!lastStop || !station || station->getMaxTracks() < 2 ||
            lastStop->getTrackUsed() != 1) continue;
        // Ưu tiên dành ray phụ cho tàu đã kết thúc tuyến để không hiển thị đậu trên ray chính.
        const int parkingEnd = lastStop->getDepartureMinute() + 60;
        const int siding = findAvailableSidingTrack(lastStop->getStationID(),
            lastStop->getArrivalMinute(), parkingEnd, station->getMaxTracks(), train);
        if (siding > 1) {
            lastStop->setTrackUsed(siding);
            addConflictLog("[Ga kết thúc " + to_string(lastStop->getStationID()) +
                           "] " + train->getTrainCode() + " đậu ray phụ " +
                           to_string(siding));
        }
    }

    MinHeap<ScheduleEvent, ScheduleEventBefore> events;
    int sequence = 0;
    for (LinkedList<TrainNode*>::Node* trainNode = trains.getHead();
         trainNode; trainNode = trainNode->next) {
        TrainNode* train = trainNode->data;
        pushTrainEvents(events, train, sequence, SAFETY_HEADWAY, 0);
    }

    HashMap<ResourceKey, ScheduleEvent, ResourceKeyHash> activeResources(32);

    while (!events.isEmpty()) {
        const ScheduleEvent current = events.top();
        events.pop();
        if (!isCurrentEvent(current)) {
            continue;
        }

        // Bảo trì được xử lý như một xung đột có thời hạn trước khi chiếm tài nguyên.
        const MaintenanceWindow* blockedByRepair = nullptr;
        int maintenanceEnd = 0;
        for (int i = 0; i < maintenanceCount; ++i) {
            const MaintenanceWindow& repair = maintenanceWindows[i];
            bool matches = false;
            if (repair.resourceType == 0 && current.type == 0) {
                matches = repair.firstID == current.stationID;
            } else if (repair.resourceType == 1 && current.type == 1) {
                const int first = min(current.stationID, current.targetStationID);
                const int second = max(current.stationID, current.targetStationID);
                matches = repair.firstID == first && repair.secondID == second;
            } else if (repair.resourceType == 2 && current.type == 0) {
                matches = repair.firstID == current.stationID &&
                          repair.secondID == current.track;
            }
            if (matches && current.time < repair.endMinute &&
                repair.startMinute < current.endTime &&
                (!blockedByRepair || repair.endMinute > maintenanceEnd)) {
                blockedByRepair = &repair;
                maintenanceEnd = repair.endMinute;
            }
        }

        if (blockedByRepair) {
            bool movedToAvailableTrack = false;
            if (current.type == 0 && blockedByRepair->resourceType == 2) {
                StationNode* station = stationList->findStationByID(current.stationID);
                if (station) {
                    for (int track = 1; track <= station->getMaxTracks(); ++track) {
                        if (track == current.track) continue;
                        bool underRepair = false;
                        for (int i = 0; i < maintenanceCount; ++i) {
                            const MaintenanceWindow& repair = maintenanceWindows[i];
                            if ((repair.resourceType == 0 && repair.firstID == current.stationID) ||
                                (repair.resourceType == 2 && repair.firstID == current.stationID &&
                                 repair.secondID == track &&
                                 current.time < repair.endMinute &&
                                 repair.startMinute < current.endTime)) {
                                underRepair = true;
                                break;
                            }
                        }
                        if (underRepair) continue;
                        if (track > 1 && findAvailableSidingTrack(current.stationID,
                                current.stop->getArrivalMinute(),
                                current.stop->getDepartureMinute(),
                                station->getMaxTracks(), current.train) != track) continue;
                        current.stop->setTrackUsed(track);
                        current.train->bumpScheduleRevision();
                        addConflictLog("[Bảo trì] " + current.train->getTrainCode() +
                            " đổi sang ray " + to_string(track) + " tại Ga " +
                            to_string(current.stationID));
                        pushTrainEvents(events, current.train, sequence,
                                        SAFETY_HEADWAY, current.time);
                        movedToAvailableTrack = true;
                        break;
                    }
                }
            }
            if (movedToAvailableTrack) continue;

            const int wait = maintenanceEnd - current.time;
            if (current.type == 0) {
                StopScheduleNode* previous = current.train->findPreviousStop(current.stop);
                if (previous) previous->setDepartureMinute(previous->getDepartureMinute() + wait);
                current.train->applyDelayPropagation(current.stationID, wait, true);
            } else {
                current.stop->setDepartureMinute(current.stop->getDepartureMinute() + wait);
                current.train->applyDelayPropagation(current.targetStationID, wait, true);
            }
            addConflictLog("[Bảo trì] " + current.train->getTrainCode() +
                " chờ " + to_string(wait) + " phút");
            current.train->bumpScheduleRevision();
            pushTrainEvents(events, current.train, sequence, SAFETY_HEADWAY, current.time);
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
                addConflictLog("[Ga " + to_string(yielding.stationID) + "] " +
                            yieldingTrain->getTrainCode() + " vào ray " + to_string(siding));
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
        } else { // type == 1 (Xung đột khu gian đường đơn)
        const int wait = priority.endTime - yielding.time;
        if (wait > 0) {
            
            StationNode* waitStation = stationList->findStationByID(yielding.stationID);
            const int waitTracks = waitStation ? waitStation->getMaxTracks() : 1;
            const int siding = findAvailableSidingTrack(
                yielding.stationID, 
                yielding.stop->getArrivalMinute(),
                yielding.stop->getDepartureMinute() + wait, // Khung giờ dự kiến đỗ mới
                waitTracks, 
                yieldingTrain
            );
            if (siding != -1) {
                yielding.stop->setTrackUsed(siding);
                yielding.stop->setDepartureMinute(yielding.stop->getDepartureMinute() + wait);
                yieldingTrain->applyDelayPropagation(yielding.targetStationID, wait, true);
            } else {
                StopScheduleNode* previous = yieldingTrain->findPreviousStop(yielding.stop);
                if (previous) {
                    previous->setDepartureMinute(previous->getDepartureMinute() + wait);
                    yieldingTrain->applyDelayPropagation(yielding.stationID, wait, true);
                } else {
                    yielding.stop->setDepartureMinute(yielding.stop->getDepartureMinute() + wait);
                    yieldingTrain->applyDelayPropagation(yielding.targetStationID, wait, true);
                }
            }
            addConflictLog("[Khu gian " + to_string(yielding.stationID) + "->" +
                        to_string(yielding.targetStationID) + "] " +
                        yieldingTrain->getTrainCode() + " chờ " +
                        priorityTrain->getTrainCode() + " +" + to_string(wait) + " phút" +
                        (siding != -1 ? ", sang ray " + to_string(siding) : ""));
            changed = true;
        }
    }

        if (changed) {
            // Thay thế tàu bị loại ra khỏi tài nguyên nếu cần
            if (!currentYields) {
                activeResources.insert(resKey, current);
            }
            yieldingTrain->bumpScheduleRevision();
            pushTrainEvents(events, yieldingTrain, sequence, SAFETY_HEADWAY, current.time);
        }
    }
}

bool TrainList::applyDelayAndReschedule(const string& trainCode, int fromStationID,
                                        int delayMinutes, StationList* stationList) {
    TrainNode* train = findTrainByCode(trainCode);
    if (!train) return false;

    addConflictLog("[Sự cố] " + trainCode + " bị trễ +" +
                   to_string(delayMinutes) + " phút từ Ga " +
                   to_string(fromStationID));
    const bool delayed =
        train->applyDelayPropagation(fromStationID, delayMinutes, true);
    if (delayed) {
        resolveConflictAndScheduleSiding(stationList);
    }
    return delayed;
}