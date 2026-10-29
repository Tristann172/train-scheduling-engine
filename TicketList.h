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

    void displayAllTickets() const; 

    // Kiểm tra ghế trống trên phân đoạn chặng theo mốc Km ga
    bool isSeatAvailable(StationList* stationList, const std::string& trainCode, 
                        int carriageNo, int seatNo, int newFromID, int newToID) const;

    // Hiển thị ma trận sơ đồ ghế
    void displaySeatMap(StationList* stationList, const std::string& trainCode,
                        int carriageNo, int totalSeats, int fromID, int toID) const;

    // Giao diện / Luồng đặt vé
    void bookTicketUI(StationList* stationList, TrainList* trainList);
};
