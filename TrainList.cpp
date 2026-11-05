#include "TrainList.h" 
#include "StationList.h"
#include "StopScheduleNode.h"
#include "StationNode.h"
#include "MinHeap.h"
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
    int endTime; // Thời gian kết thúc sự kiện + SAFETY_TIMES
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

bool sameResource(const ScheduleEvent& left, const ScheduleEvent& right) {
    if (left.type != right.type) return false;
    if (left.type == 0) {
        return left.stationID == right.stationID && left.track == right.track;
    }
    return (left.stationID == right.stationID &&
            left.targetStationID == right.targetStationID) ||
           (left.stationID == right.targetStationID &&
            left.targetStationID == right.stationID);
}

struct ResourceSlot {
    ScheduleEvent event;
    int state; // 0: empty, 1: occupied, 2: deleted

    ResourceSlot() : state(0) {}
};

int resourceHash(const ScheduleEvent& event, int capacity) {
    int firstStation = event.stationID;
    int secondStation = event.targetStationID;
    if (event.type == 1 && firstStation > secondStation) {
        const int temp = firstStation;
        firstStation = secondStation;
        secondStation = temp;
    }

    unsigned long hash = static_cast<unsigned long>(event.type + 1);
    hash = hash * 16777619UL + static_cast<unsigned long>(firstStation);
    hash = hash * 16777619UL + static_cast<unsigned long>(secondStation);
    hash = hash * 16777619UL + static_cast<unsigned long>(event.track);
    return static_cast<int>(hash % static_cast<unsigned long>(capacity));
}

// Tìm index của event nằm trong slots, nếu có thì trả về index, còn nếu kh thì trả về index
// hợp lí để add event vào
int findResourceSlot(ResourceSlot* slots, int capacity,
                     const ScheduleEvent& event, bool& found) {
    int index = resourceHash(event, capacity);
    int firstDeleted = -1;
    found = false;

    for (int probes = 0; probes < capacity; ++probes) {
        if (slots[index].state == 0) {
            return firstDeleted >= 0 ? firstDeleted : index;
        }
        if (slots[index].state == 2) {
            if (firstDeleted < 0) firstDeleted = index;
        } else if (sameResource(slots[index].event, event)) {
            found = true;
            return index;
        }
        index = (index + 1) % capacity;
    }
    return firstDeleted;
}

void growResourceTable(ResourceSlot*& slots, int& capacity, int& activeCount) {
    const int newCapacity = capacity * 2;
    ResourceSlot* expanded = new ResourceSlot[newCapacity];
    int retainedCount = 0;
    for (int i = 0; i < capacity; ++i) {
        if (slots[i].state != 1 || !isCurrentEvent(slots[i].event)) {
            continue;
        }
        const int start = resourceHash(slots[i].event, newCapacity);
        int index = start;
        while (expanded[index].state == 1) {
            index = (index + 1) % newCapacity;
        }
        expanded[index] = slots[i];
        ++retainedCount;
    }
    delete[] slots;
    slots = expanded;
    capacity = newCapacity;
    activeCount = retainedCount;
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

    int resourceCapacity = 8;
    ResourceSlot* activeResources = new ResourceSlot[resourceCapacity];
    int activeResourceCount = 0;

    while (!events.isEmpty()) {
        const ScheduleEvent current = events.top();
        events.pop();
        if (!isCurrentEvent(current)) {
            continue;
        }

        bool resourceFound = false;
        const int resourceIndex = findResourceSlot(activeResources,
                                                   resourceCapacity,
                                                   current, resourceFound);
        if (resourceIndex < 0) {
            delete[] activeResources;
            addConflictLog("[Loi] Khong the cap phat o trang thai tai nguyen lich tau.");
            return;
        }

        if (resourceFound &&
            (!isCurrentEvent(activeResources[resourceIndex].event) ||
             activeResources[resourceIndex].event.endTime <= current.time)) {
            activeResources[resourceIndex].state = 2;
            --activeResourceCount;
            resourceFound = false;
        }

        if (!resourceFound) {
            if ((activeResourceCount + 1) * 4 > resourceCapacity * 3) {
                growResourceTable(activeResources, resourceCapacity,
                                  activeResourceCount);
                const int expandedIndex = findResourceSlot(
                    activeResources, resourceCapacity, current, resourceFound);
                if (expandedIndex < 0) {
                    delete[] activeResources;
                    addConflictLog("[Loi] Khong the cap phat o trang thai tai nguyen lich tau.");
                    return;
                }
                if (resourceFound) {
                    activeResources[expandedIndex].event = current;
                    activeResources[expandedIndex].state = 1;
                    continue;
                }
                if (activeResources[expandedIndex].state == 0) {
                    ++activeResourceCount;
                }
                activeResources[expandedIndex].event = current;
                activeResources[expandedIndex].state = 1;
                continue;
            }
            if (activeResources[resourceIndex].state == 0) {
                ++activeResourceCount;
            }
            activeResources[resourceIndex].event = current;
            activeResources[resourceIndex].state = 1;
            continue;
        }

        const ScheduleEvent other = activeResources[resourceIndex].event;
        if (other.train == current.train) {
            if (current.endTime > activeResources[resourceIndex].event.endTime) {
                activeResources[resourceIndex].event.endTime = current.endTime;
            }
            continue;
        }

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
                // FIX LỖI 1: Ngay cả khi có ray phụ, nếu tàu phải chờ tàu khác (ví dụ chờ SE2 đi qua), 
                // nó VẪN PHẢI bị delay giờ xuất phát để không lao ra khu gian cùng lúc.
                const int wait = priority.endTime - yielding.time;
                if (wait > 0 && priority.type == 1) { // Nếu đang nhường một tàu chạy khu gian
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
                        // ... (phần code gốc tìm siding cho previous)
                    }
                    yieldingTrain->applyDelayPropagation(yielding.stationID, wait, true);
                    changed = true;
                }
            }
        } else { // type == 1 (Khu gian)
            // FIX LỖI 2: Đảm bảo khoảng cách an toàn (Headway) cho tàu chạy cùng chiều
            int wait = priority.endTime - yielding.time;
            
            // Nếu 2 tàu chạy CÙNG CHIỀU (vào cùng một khu gian, ga đích giống nhau)
            if (yielding.targetStationID == priority.targetStationID) {
                // Tàu chạy sau phải đợi tàu trước TỚI GA ĐÍCH + HEADWAY mới được xuất phát
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
            if (!currentYields) {
                activeResources[resourceIndex].event = current;
                activeResources[resourceIndex].state = 1;
            }
            yieldingTrain->bumpScheduleRevision();
            pushTrainEvents(events, yieldingTrain, sequence, SAFETY_HEADWAY);
        }
    }
    delete[] activeResources;
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
