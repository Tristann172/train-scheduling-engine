#pragma once

#include <SFML/Graphics.hpp>
#include <string>

class StationList;
class TrainList;

class GraphicalSimulationEngine {
private:
    static const unsigned int WINDOW_WIDTH = 1360;
    static const unsigned int WINDOW_HEIGHT = 768;

public:
    // Chạy mô phỏng điều phối chạy tàu & đường ray 2D bằng SFML
    static void runSFMLSimulation(StationList* stationList, TrainList* trainList);
};