#include "StationList.h"

StationList::StationList() : head(nullptr), tail(nullptr) {}

void StationList::clear() {
    while (head != nullptr) {
        StationNode* temp = head;
        head = head->getNextStation();
        delete temp;
    }
    head = nullptr;
    tail = nullptr;
}

StationList::~StationList() {
    clear();
}

StationNode* StationList::getHead() const { return head; }
StationNode* StationList::getTail() const { return tail; }

void StationList::addStation(int id, const std::string& name, double km, int tracks) {
    StationNode* newNode = new StationNode(id, name, km, tracks);
    if (head == nullptr) {
        head = tail = newNode;
        return;
    }
    tail->setNextStation(newNode);
    tail = newNode;
}

StationNode* StationList::findStationByID(int id) const {
    StationNode* temp = head;
    while (temp != nullptr) {
        if (temp->getStationID() == id) return temp;
        temp = temp->getNextStation();
    }
    return nullptr;
}