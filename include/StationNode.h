#pragma once
#include <string>

struct Station {
    int stationID;
    std::string name;
    double kmMarker;
    int maxTracks; // Năng lực hạ tầng: Tổng số đường ray tại ga (1 = ga xép chỉ có ray chính)
public:
    Station(int id, const std::string& stName, double km, int tracks)
        : stationID(id), name(stName), kmMarker(km), 
          maxTracks(tracks < 1 ? 1 : tracks) {}

    int getStationID() const { return stationID; }
    std::string getName() const { return name; }
    double getKmMarker() const { return kmMarker; }
    
    int getMaxTracks() const { return maxTracks; }
    int getNumberOfTracks() const { return maxTracks; } // Giữ tương thích với FileManager & GuiMenu
    void setMaxTracks(int tracks) { maxTracks = (tracks < 1) ? 1 : tracks; }

};

typedef Station StationNode;