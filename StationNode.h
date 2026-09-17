#ifndef STATION_NODE_H
#define STATION_NODE_H

#include <string>

class StationNode {
private:
    int stationID;
    std::string name;
    double kmMarker;
    int numberOfTracks;
    StationNode* nextStation; // Con tr? tr? d?n node Ga ti?p theo

public:
    StationNode(int id, const std::string& stName, double km, int tracks)
        : stationID(id), name(stName), kmMarker(km), numberOfTracks(tracks), nextStation(NULL) {}

    // Getters & Setters
    int getStationID() const { return stationID; }
    std::string getName() const { return name; }
    double getKmMarker() const { return kmMarker; }
    int getNumberOfTracks() const { return numberOfTracks; }
    StationNode* getNextStation() const { return nextStation; }

    void setNextStation(StationNode* next) { nextStation = next; }
};

#endif
