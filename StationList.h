#pragma once
#include <iostream>
#include <string>
#include "StationNode.h"

class StationList {
private:
    StationNode* head;
    StationNode* tail; // Con trỏ cuối giúp chèn O(1)

public:
    StationList();
    ~StationList();

    StationNode* getHead() const;
    StationNode* getTail() const;
    void addStation(int id, const std::string& name, double km, int tracks);
    StationNode* findStationByID(int id) const;
    void clear();
};