#pragma once
#include <string>
#include "TicketNode.h"

class StationList; 
class StationNode;

class TicketList {
private:
    TicketNode* head;

public:
    TicketList();
    
    ~TicketList();
    void clear();

    TicketNode* getHead() const;
    void addTicket(const std::string& ticketID, const std::string& trainCode, int carriageNo, int seatNo, int fromID, int toID);
    TicketNode* findTicketByID(const std::string& ticketID) const;
    bool cancelTicket(const std::string& ticketID);
    void displayAllTickets() const; 
    bool isSeatAvailable(StationList* stationList, const std::string& trainCode, int carriageNo, int seatNo, int newFromID, int newToID) const;
    void displaySeatMap(StationList* stationList, const std::string& trainCode, int carriageNo, int totalSeats, int fromID, int toID) const;
    void bookTicketUI(StationList* stationList, TrainList* trainList);
};
