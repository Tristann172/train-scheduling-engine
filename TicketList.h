#pragma once

#include <string>
#include <iostream>
#include "TicketNode.h"

class StationList;
class TrainList;

class TicketList {
private:
    TicketNode* head;

public:
    TicketList();
    ~TicketList();

    TicketNode* getHead() const { return head; }
    void clear();

    void addTicket(const std::string& ticketID, const std::string& trainCode, 
                   int carriageNo, int seatNo, int fromID, int toID);

    TicketNode* findTicketByID(const std::string& ticketID) const;

    bool cancelTicket(const std::string& ticketID);

    // Kiểm tra ghế trống trên phân đoạn chặng theo mốc Km ga
    bool isSeatAvailable(StationList* stationList, TrainList* trainList, const std::string& trainCode, 
                     int carriageNo, int seatNo, int newFromID, int newToID) const;

};
