#pragma once

#include <iostream>
#include <string>
#include "StationNode.h"

class StationList {
private:
    StationNode* head;

public:
    StationList() : head(NULL) {}

    ~StationList() {
        while (head != NULL) {
            StationNode* temp = head;
            head = head->getNextStation();
            delete temp;
        }
    }

    StationNode* getHead() const { return head; }

    void addStation(int id, const std::string& name, double km, int tracks) {
        StationNode* newNode = new StationNode(id, name, km, tracks);
        if (head == NULL) {
            head = newNode;
            return;
        }
        StationNode* temp = head;
        while (temp->getNextStation() != NULL) {
            temp = temp->getNextStation();
        }
        temp->setNextStation(newNode);
    }

    StationNode* findStationByID(int id) const {
        StationNode* temp = head;
        while (temp != NULL) {
            if (temp->getStationID() == id) return temp;
            temp = temp->getNextStation();
        }
        return NULL;
    }

    void displayAll() const {
        std::cout << "\n=== DANH SACH CAC GA TREN TUYEN ===\n";
        StationNode* temp = head;
        while (temp != NULL) {
            std::cout << "ID: " << temp->getStationID() 
                      << " | Ten: " << temp->getName() 
                      << " | Km: " << temp->getKmMarker() 
                      << " | So ray phu: " << temp->getNumberOfTracks() << "\n";
            temp = temp->getNextStation();
        }
    }
};