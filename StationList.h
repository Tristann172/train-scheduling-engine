#pragma once
#include <iostream>
#include <string>
#include "StationNode.h"
#include "LinkedList.h"
#include "HashMap.h"

class StationList {
private:
    LinkedList<Station*> stations;
    HashMap<int, Station*, IntHash> stationsByID;

public:
    StationList();
    ~StationList();

    LinkedList<Station*>::Node* getHead() const;
    LinkedList<Station*>::Node* getTail() const;
    int getSize() const { return stations.getSize(); }
    void addStation(int id, const std::string& name, double km, int tracks);
    StationNode* findStationByID(int id) const;
    void clear();
};