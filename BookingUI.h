#pragma once
#include "TicketList.h"
#include "StationList.h"
#include "TrainList.h"
#include <string>

class BookingUI {
public:
    static void displayAllTickets(const TicketList& ticketList);
    
    static void displaySeatMap(const TicketList& ticketList, StationList* stationList, 
                               const std::string& trainCode, int carriageNo, 
                               int totalSeats, int fromID, int toID);
                               
    static void bookTicketUI(TicketList& ticketList, StationList* stationList, TrainList* trainList);
};