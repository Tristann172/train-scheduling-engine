#pragma once

#include <iostream>
#include <string>
#include "TrainNode.h"

class StationList;

class TrainList {
private:
    TrainNode* head;

public:
    TrainList();
    ~TrainList();
    void clear(); // Dọn dẹp bộ nhớ dynamic
    
    // Các hàm quản lý danh sách theo cấu trúc gốc của bạn
    TrainNode* getHead() const;
    void addTrain(const std::string& code, int direction);
    TrainNode* findTrainByCode(const std::string& code) const;
    void addCarriageToTrain(const std::string& code, int cNo, const std::string& type, int seats);
    void addStopScheduleToTrain(const std::string& code, int stID, int arr, int dep, int track);

    // Bổ sung: Hiển thị danh sách tàu
    void displayAllTrains() const;
    
    // Bổ sung: Thuật toán Tái lập lịch tránh tàu / Phân bổ ray phụ khi phát hiện xung đột
    void resolveConflictAndScheduleSiding(StationList* stationList);

    // Bổ sung: Kích hoạt Lan truyền trễ + Tự động tái lập lịch tránh tàu liên đới
    bool applyDelayAndReschedule(const std::string& trainCode, int fromStationID, int delayMinutes, StationList* stationList);
};
