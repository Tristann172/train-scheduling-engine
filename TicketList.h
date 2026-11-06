#pragma once
#include <string>
#include <iostream>
#include "TicketNode.h"
#include "LinkedList.h"
#include "HashMap.h"

class StationList;
class TrainList;

class TicketList {
private:
    LinkedList<Ticket*> tickets;
    HashMap<std::string, Ticket*, StringHash> ticketsByID;
    unsigned long occupancyRevision;
    mutable unsigned long builtOccupancyRevision;
    mutable unsigned long builtTrainRevision;
    mutable TrainList* occupancyTrainList;

    void ensureSeatOccupancy(TrainList* trainList) const;
    void updateSeatOccupancyForTicket(const TicketNode* ticket, int delta) const;

public:
    TicketList();
    ~TicketList();

    LinkedList<Ticket*>::Node* getHead() const { return tickets.getHead(); }
    int getSize() const { return tickets.getSize(); }
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