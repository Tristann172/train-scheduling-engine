#include "TicketList.h"
#include "StationList.h"
#include <iostream>

TicketList::TicketList() : head(nullptr) {}

void TicketList::clear() {
    while (head != nullptr) {
        TicketNode* temp = head;
        head = head->getNextTicket();
        delete temp; 
    }
    head = nullptr;
}

TicketList::~TicketList() {
    clear();
}

TicketNode* TicketList::getHead() const {
    return head;
}

void TicketList::addTicket(const std::string& ticketID, const std::string& trainCode,
                           int carriageNo, int seatNo, int fromID, int toID) {
    TicketNode* newNode = new TicketNode(ticketID, trainCode, carriageNo, seatNo, fromID, toID);
    
    if (head == NULL) {
        head = newNode;
    } else {
        TicketNode* current = head;
        while (current->getNextTicket() != NULL) {
            current = current->getNextTicket();
        }
        current->setNextTicket(newNode);
    }
}

bool TicketList::isSeatAvailable(StationList* stationList, const std::string& trainCode,
                                 int carriageNo, int seatNo, int newFromID, int newToID) const {
    
    StationNode* newFromSt = stationList->findStationByID(newFromID);
    StationNode* newToSt = stationList->findStationByID(newToID);

    if (newFromSt == NULL || newToSt == NULL) return false; 

    double newKm1 = newFromSt->getKmMarker();
    double newKm2 = newToSt->getKmMarker();
    double newMinKm = (newKm1 < newKm2) ? newKm1 : newKm2;
    double newMaxKm = (newKm1 > newKm2) ? newKm1 : newKm2;

    TicketNode* current = head;
    while (current != NULL) {
        if (current->getTrainCode() == trainCode &&
            current->getCarriageNo() == carriageNo &&
            current->getSeatNo() == seatNo) {

            StationNode* existFromSt = stationList->findStationByID(current->getFromStationID());
            StationNode* existToSt = stationList->findStationByID(current->getToStationID());

            if (existFromSt != NULL && existToSt != NULL) {
                double existKm1 = existFromSt->getKmMarker();
                double existKm2 = existToSt->getKmMarker();
                double existMinKm = (existKm1 < existKm2) ? existKm1 : existKm2;
                double existMaxKm = (existKm1 > existKm2) ? existKm1 : existKm2;

                if (!(existMaxKm <= newMinKm || existMinKm >= newMaxKm)) {
                    return false; 
                }
            }
        }
        current = current->getNextTicket();
    }
    return true; 
}
