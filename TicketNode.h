#ifndef TICKET_NODE_H
#define TICKET_NODE_H

#include <string>

class TicketNode {
private:
    std::string ticketID;
    std::string trainCode;
    int carriageNo;
    int seatNo;
    int fromStationID;
    int toStationID;
    TicketNode* nextTicket;

public:
    TicketNode(const std::string& tID, const std::string& tCode, int cNo, int sNo, int fromID, int toID)
        : ticketID(tID), trainCode(tCode), carriageNo(cNo), seatNo(sNo), 
          fromStationID(fromID), toStationID(toID), nextTicket(nullptr) {}

    std::string getTicketID() const { return ticketID; }
    std::string getTrainCode() const { return trainCode; }
    int getCarriageNo() const { return carriageNo; }
    int getSeatNo() const { return seatNo; }
    int getFromStationID() const { return fromStationID; }
    int getToStationID() const { return toStationID; }
    TicketNode* getNextTicket() const { return nextTicket; }

    void setNextTicket(TicketNode* next) { nextTicket = next; }
};

#endif
