#pragma once

#include "MapData.h"
#include "UiKit.h"

#include <SFML/Graphics.hpp>
#include <string>

class StationList;
class TrainList;

const int SIM_MAX_STATIONS = 128;
const int SIM_MAX_TRAINS = 48;
const int SIM_MAX_STOPS = 128;
const int SIM_MAX_WINDOWS = 128;

std::string formatMinute(double minute);
std::string formatDuration(int minutes);

struct SimStation {
    int id;
    std::string name;
    double km;
    int tracks;
    char side;
    bool major;
};

struct SimStop {
    int stationID;
    int arrival;
    int departure;
    int track;
    int baseDeparture; // Giờ đi theo lịch công bố
    bool hasBase;
};

struct SimTrain {
    std::string code;
    int direction;
    int priority;
    int stopCount;
    SimStop stops[SIM_MAX_STOPS];
};

struct SimWindow {
    int type; // 0: ga, 1: khu gian, 2: ray
    int first;
    int second;
    int start;
    int end;
    std::string label;
};

struct SimEvent {
    int minute;
    std::string text;
};

enum class TrainMode { Waiting, Origin, Running, Stopped, Holding, Arrived };

struct TrainStatus {
    TrainMode mode;
    int stop;
    double km;
    int delay;
    std::string brief;
    std::string line1;
    std::string line2;
    sf::Color color;
    bool dashed;
};

// Ảnh chụp dữ liệu để vẽ, dựng lại sau mỗi lần dữ liệu thay đổi.
class SimModel {
public:
    SimModel();
    void rebuild(const StationList& stations, const TrainList& trains,
                 const TrainList& baseline, const MapData& map);

    int stationCount() const { return stations; }
    const SimStation& station(int index) const { return stationRows[index]; }
    const SimStation* findStation(int id) const;
    std::string nameOf(int id) const;
    double kmOf(int id) const;

    int trainCount() const { return trains; }
    const SimTrain& train(int index) const { return trainRows[index]; }
    int windowCount() const { return windows; }
    const SimWindow& window(int index) const { return windowRows[index]; }

    int firstMinute() const { return earliest; }
    int lastMinute() const { return latest; }

    TrainStatus status(int trainIndex, double minute) const;
    int stopDelay(const SimTrain& train, int stop) const;
    int addedWait(const SimTrain& train, int stop) const;
    std::string waitReason(int trainIndex, int stop, int reference, bool longForm) const;

    bool stationClosed(int id, double minute) const;
    bool blockClosed(int a, int b, double minute) const;
    int activeWindowCount(double minute) const;
    int upcoming(double minute, SimEvent* out, int capacity) const;

private:
    SimStation stationRows[SIM_MAX_STATIONS];
    int stations;
    SimTrain trainRows[SIM_MAX_TRAINS];
    int trains;
    SimWindow windowRows[SIM_MAX_WINDOWS];
    int windows;
    int earliest;
    int latest;
};

class GraphicalSimulationEngine {
public:
    GraphicalSimulationEngine();

    void resetClock(const SimModel& model);
    void update(Ui::Canvas& ui, const SimModel& model);
    double minute() const { return currentMinute; }

    // Bản đồ tĩnh của màn Nhà ga.
    void drawStaticMap(Ui::Canvas& ui, const MapData& map, const SimModel& model, float x, float y);
    // Màn Bản đồ mô phỏng. Trả về true khi người dùng chuyển sang Đồ thị.
    bool drawScreen(Ui::Canvas& ui, const MapData& map, const SimModel& model);

private:
    double currentMinute;
    bool playing;
    int speedIndex;
    int zoomIndex;
    float viewScale;
    float viewX;
    float viewY;
    bool dragging;
    int firstRow;

    void drawMap(Ui::Canvas& ui, const MapData& map, const SimModel& model, float x, float y, bool full);
    void drawControls(Ui::Canvas& ui, const SimModel& model);
    void drawPanel(Ui::Canvas& ui, const SimModel& model);
};
