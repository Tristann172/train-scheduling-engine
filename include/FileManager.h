#pragma once

#include <string>

class StationList;
class TrainList;

class FileManager {
public:
    static void loadStations(const std::string& filename, StationList& stationList);
    static bool saveStations(const std::string& filename, const StationList& stationList);

    static void loadTrains(const std::string& filename, TrainList& trainList);
    static bool saveTrains(const std::string& filename, const TrainList& trainList);
};
