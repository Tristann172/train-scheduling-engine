#pragma once

#include <iostream>
#include <string>
#include "StationNode.h"

class StationList {
private:
    StationNode* head;

public:
    StationList();
    ~StationList();

    StationNode* getHead() const;
    void addStation(int id, const std::string& name, double km, int tracks);
    StationNode* findStationByID(int id) const;
    void displayAll() const;
    void clear();
};