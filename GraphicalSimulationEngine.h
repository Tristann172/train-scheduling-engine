#pragma once

#include <SFML/Graphics.hpp>
#include <string>

class StationList;
class TrainList;

class GraphicalSimulationEngine {
private:
    static const int WINDOW_WIDTH = 1280;
    static const int WINDOW_HEIGHT = 720;

public:
    // Khai báo hàm chạy mô phỏng chạy tàu & đường ray 2D bằng SFML
    static void runSFMLSimulation(StationList* stationList, TrainList* trainList);
};
