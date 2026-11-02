#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "TrainNode.h"

class StationList;

class TrainList {
private:
    TrainNode* head;
    TrainNode* tail;

    // Thay thế vector bằng Circular Buffer tĩnh (tối đa 8 thông báo)
    static const int MAX_LOGS = 8;
    std::string conflictLogs[MAX_LOGS];
    int logStart; // Vị trí phần tử cũ nhất
    int logCount; // Số lượng thông báo hiện có (0..MAX_LOGS)

    // Hàm phụ trợ: Tìm một đường ray phụ còn trống tại ga trong khoảng [arr, dep]
    int findAvailableSidingTrack(int stationID, int arr, int dep, int maxTracks, TrainNode* excludeTrain) const;

public:
    TrainList();
    ~TrainList();
    void clear();

    TrainNode* getHead() const;
    void addTrain(const std::string& code, int direction, int priority = -1);
    TrainNode* findTrainByCode(const std::string& code) const;
    void addCarriageToTrain(const std::string& code, int cNo, const std::string& type, int seats);
    void addStopScheduleToTrain(const std::string& code, int stID, int arr, int dep, int track);

    void resolveConflictAndScheduleSiding(StationList* stationList);
    bool applyDelayAndReschedule(const std::string& trainCode, int fromStationID, int delayMinutes, StationList* stationList);

    // Giao diện truy xuất log cho màn hình HUD (không dùng vector)
    void addConflictLog(const std::string& logMsg);
    int getLogCount() const { return logCount; }
    std::string getLog(int index) const; // Lấy dòng log thứ index (0: cũ nhất -> logCount - 1: mới nhất)
};