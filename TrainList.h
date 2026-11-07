#pragma once
#include <iostream>
#include <string>
#include "TrainNode.h"
#include "LinkedList.h"
#include "HashMap.h"

class StationList;

struct MaintenanceWindow {
    int resourceType;
    int firstID;
    int secondID;
    int startMinute;
    int endMinute;
};

class TrainList {
private:
    LinkedList<Train*> trains;
    HashMap<std::string, Train*, StringHash> trainsByCode;
    unsigned long structureRevision;

    // Thay thế vector bằng Circular Buffer tĩnh (tối đa 8 thông báo)
    static const int MAX_LOGS = 8;
    std::string conflictLogs[MAX_LOGS];
    int logStart; // Vị trí phần tử cũ nhất
    int logCount; // Số lượng thông báo hiện có (0..MAX_LOGS)
    static const int MAX_MAINTENANCE_WINDOWS = 128;
    MaintenanceWindow maintenanceWindows[MAX_MAINTENANCE_WINDOWS];
    int maintenanceCount;

    // Hàm phụ trợ: Tìm một đường ray phụ còn trống tại ga trong khoảng [arr, dep]
    int findAvailableSidingTrack(int stationID, int arr, int dep, int maxTracks, TrainNode* excludeTrain) const;

public:
    TrainList();
    ~TrainList();
    void clear();

    LinkedList<Train*>::Node* getHead() const;
    int getSize() const { return trains.getSize(); }
    unsigned long getStructureRevision() const { return structureRevision; }
    bool addTrain(const std::string& code, int direction, int priority = -1);
    TrainNode* findTrainByCode(const std::string& code) const;
    bool updateTrain(const std::string& code, int direction, int priority,
                     const StationList* stationList);
    bool removeTrain(const std::string& code);
    bool addCarriageToTrain(const std::string& code, int cNo, const std::string& type);
    bool updateCarriage(const std::string& code, int cNo, const std::string& type);
    bool removeCarriage(const std::string& code, int cNo);
    bool addStopScheduleToTrain(const std::string& code, int stID, int arr, int dep,
                                int track, StationList* stationList);
    bool updateStopSchedule(const std::string& code, int stID, int arr, int dep,
                            int track, const StationList* stationList);
    bool removeStopSchedule(const std::string& code, int stID);
    bool hasStationStop(int stationID) const;
    bool hasTrackAbove(int stationID, int track) const;
    bool hasMaintenanceForStation(int stationID) const;
    bool hasMaintenanceBlockForStation(int stationID) const;
    bool hasMaintenanceTrackAbove(int stationID, int track) const;
    bool addMaintenance(const std::string& resource, int firstID, int secondID,
                        int startMinute, int endMinute);
    bool removeMaintenance(const std::string& resource, int firstID, int secondID,
                           int startMinute, int endMinute);
    bool isUnderMaintenanceAt(const std::string& resource, int firstID, int secondID,
                              int minute) const;

    void resolveConflictAndScheduleSiding(StationList* stationList);
    bool applyDelayAndReschedule(const std::string& trainCode, int fromStationID, int delayMinutes, StationList* stationList);

    // Giao diện truy xuất log cho màn hình HUD (không dùng vector)
    void addConflictLog(const std::string& logMsg);
    int getLogCount() const { return logCount; }
    std::string getLog(int index) const; // Lấy dòng log thứ index (0: cũ nhất -> logCount - 1: mới nhất)
};