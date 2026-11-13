#include "StationList.h"
#include <cmath>

StationList::StationList() : stations(), stationsByID() {}

void StationList::clear() {
    LinkedList<Station*>::Node* station = stations.getHead();
    while (station) {
        delete station->data;
        station = station->next;
    }
    stations.clear();
    stationsByID.clear();
}

StationList::~StationList() {
    clear();
}

LinkedList<Station*>::Node* StationList::getHead() const {
    return stations.getHead();
}

LinkedList<Station*>::Node* StationList::getTail() const {
    return stations.getTail();
}

bool StationList::addStation(int id, const std::string& name, double km, int tracks) {
    if (id <= 0 || name.empty() || !std::isfinite(km) || km < 0 || tracks < 1 ||
        stationsByID.find(id)) return false;

    StationNode* newNode = new StationNode(id, name, km, tracks);

    stations.pushBack(newNode);
    stationsByID.insert(id, newNode);
    return true;
}

StationNode* StationList::findStationByID(int id) const {
    StationNode* const* station = stationsByID.find(id);
    return station ? *station : nullptr;
}

bool StationList::updateStation(int id, const std::string& name, double km, int tracks) {
    StationNode* station = findStationByID(id);
    if (!station || name.empty() || !std::isfinite(km) || km < 0 || tracks < 1) return false;
    station->name = name;
    station->kmMarker = km;
    station->setMaxTracks(tracks);
    return true;
}

bool StationList::removeStation(int id) {
    StationNode* station = findStationByID(id);
    if (!station) return false;
    LinkedList<Station*>::Node* current = stations.getHead();
    while (current && current->data != station) current = current->next;
    if (!current || !stations.removeNode(current)) return false;
    stationsByID.remove(id);
    delete station;
    return true;
}