#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "TicketNode.h"

class StationList;
class TrainList;

class TicketList {
private:
    TicketNode* head;
    TicketNode* tail;

public:
    TicketList();
    ~TicketList();

    TicketNode* getHead() const { return head; }
    void clear();

    void addTicket(const std::string& ticketID, const std::string& trainCode, 
                   int carriageNo, int seatNo, int fromID, int toID);
    TicketNode* findTicketByID(const std::string& ticketID) const;
    bool cancelTicket(const std::string& ticketID);

    bool isSeatAvailable(StationList* stationList, TrainList* trainList, const std::string& trainCode, 
                         int carriageNo, int seatNo, int newFromID, int newToID) const;

    // Truyền một mảng bool có sẵn vào hàm để điền dữ liệu (không cần vector)
    bool checkCarriageSeats(StationList* stationList, TrainList* trainList,
                            const std::string& trainCode, int carriageNo,
                            int newFromID, int newToID,
                            bool seatFreeOut[], int maxSeats) const;
};