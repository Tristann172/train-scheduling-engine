#include "StationList.h"

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

void StationList::addStation(int id, const std::string& name, double km, int tracks) {
    StationNode* newNode = new StationNode(id, name, km, tracks);
    stations.pushBack(newNode);
    if (!stationsByID.find(id)) {
        stationsByID.insert(id, newNode);
    }
}

StationNode* StationList::findStationByID(int id) const {
    StationNode* const* station = stationsByID.find(id);
    return station ? *station : nullptr;
}