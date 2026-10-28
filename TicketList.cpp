#include "TicketList.h"
#include "StationList.h"
#include "TrainList.h"
#include <cstddef> // cho NULL / nullptr

using namespace std;

TicketList::TicketList() : head(nullptr) {}

TicketList::~TicketList() {
    clear();
}

void TicketList::clear() {
    while (head != nullptr) {
        TicketNode* temp = head;
        head = head->nextTicket;
        delete temp; 
    }
}

TicketNode* TicketList::getHead() const {
    return head;
}

bool TicketList::addTicket(const std::string& ticketID, const std::string& trainCode,
                           int carriageNo, int seatNo, int fromID, int toID) {
    if (findTicketByID(ticketID) != nullptr) {
        return false;
    }

    TicketNode* newNode = new TicketNode(ticketID, trainCode, carriageNo, seatNo, fromID, toID);
    
    if (head == nullptr) {
        head = newNode;
    } else {
        TicketNode* current = head;
        while (current->nextTicket != nullptr) {
            current = current->nextTicket;
        }
        current->nextTicket = newNode;
    }
    return true;
}

bool TicketList::isSeatAvailable(StationList* stationList, const std::string& trainCode,
                                 int carriageNo, int seatNo, int newFromID, int newToID) const {
    
    StationNode* newFromSt = stationList->findStationByID(newFromID);
    StationNode* newToSt = stationList->findStationByID(newToID);

    if (newFromSt == nullptr || newToSt == nullptr) return false; 

    double newKm1 = newFromSt->getKmMarker();
    double newKm2 = newToSt->getKmMarker();
    double newMinKm = (newKm1 < newKm2) ? newKm1 : newKm2;
    double newMaxKm = (newKm1 > newKm2) ? newKm1 : newKm2;

    TicketNode* current = head;
    while (current != nullptr) {
        if (current->trainCode == trainCode &&
            current->carriageNo == carriageNo &&
            current->seatNo == seatNo) {

            StationNode* existFromSt = stationList->findStationByID(current->fromStationID);
            StationNode* existToSt = stationList->findStationByID(current->toStationID);

            if (existFromSt != nullptr && existToSt != nullptr) {
                double existKm1 = existFromSt->getKmMarker();
                double existKm2 = existToSt->getKmMarker();
                double existMinKm = (existKm1 < existKm2) ? existKm1 : existKm2;
                double existMaxKm = (existKm1 > existKm2) ? existKm1 : existKm2;

                if (!(existMaxKm <= newMinKm || existMinKm >= newMaxKm)) {
                    return false; 
                }
            }
        }
        current = current->nextTicket;
    }
    return true; 
}

TicketNode* TicketList::findTicketByID(const std::string& ticketID) const {
    TicketNode* current = head;
    while (current != nullptr) {
        if (current->ticketID == ticketID) {
            return current;
        }
        current = current->nextTicket;
    }
    return nullptr;
}

bool TicketList::cancelTicket(const std::string& ticketID) {
    if (head == nullptr) return false;
    
    if (head->ticketID == ticketID) {
        TicketNode* temp = head;
        head = head->nextTicket;
        delete temp; 
        return true;
    }
    
    TicketNode* current = head;
    while (current->nextTicket != nullptr) {
        if (current->nextTicket->ticketID == ticketID) {
            TicketNode* temp = current->nextTicket;
            current->nextTicket = temp->nextTicket;
            delete temp; 
            return true;
        }
        current = current->nextTicket;
    }
    return false;
}