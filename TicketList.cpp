#include "TicketList.h"
#include "StationList.h"
#include "TrainList.h"
#include "StationNode.h"
#include "TrainNode.h"
#include <iostream>
#include <algorithm>

using namespace std;

TicketList::TicketList() : head(nullptr) {}

TicketList::~TicketList() {
    clear();
}

void TicketList::clear() {
    while (head != nullptr) {
        TicketNode* temp = head;
        head = head->getNextTicket();
        delete temp;
    }
    head = nullptr;
}

void TicketList::addTicket(const string& ticketID, const string& trainCode, 
                           int carriageNo, int seatNo, int fromID, int toID) {
    TicketNode* newNode = new TicketNode(ticketID, trainCode, carriageNo, seatNo, fromID, toID);
    if (!head) {
        head = newNode;
        return;
    }
    TicketNode* temp = head;
    while (temp->getNextTicket()) {
        temp = temp->getNextTicket();
    }
    temp->setNextTicket(newNode);
}

TicketNode* TicketList::findTicketByID(const string& ticketID) const {
    TicketNode* temp = head;
    while (temp != nullptr) {
        if (temp->getTicketID() == ticketID) return temp;
        temp = temp->getNextTicket();
    }
    return nullptr;
}

bool TicketList::cancelTicket(const string& ticketID) {
    if (!head) return false;

    if (head->getTicketID() == ticketID) {
        TicketNode* temp = head;
        head = head->getNextTicket();
        delete temp;
        return true;
    }

    TicketNode* curr = head;
    while (curr->getNextTicket() && curr->getNextTicket()->getTicketID() != ticketID) {
        curr = curr->getNextTicket();
    }

    if (curr->getNextTicket()) {
        TicketNode* temp = curr->getNextTicket();
        curr->setNextTicket(temp->getNextTicket());
        delete temp;
        return true;
    }
    return false;
}

// ==================== SEGMENT BOOKING (KIỂM TRA GIAO THOA KM) ====================
bool TicketList::isSeatAvailable(StationList* stationList, TrainList* trainList, const string& trainCode, 
                                int carriageNo, int seatNo, int newFromID, int newToID) const 
{
    // 1. Kiểm tra con trỏ danh sách hợp lệ
    if (!stationList || !trainList) return false;

    // 2. Ga đi và ga đến không được phép trùng nhau
    if (newFromID == newToID) return false;

    // 3. Kiểm tra 2 ga có tồn tại trong hệ thống không
    StationNode* stNewFrom = stationList->findStationByID(newFromID);
    StationNode* stNewTo = stationList->findStationByID(newToID);
    if (!stNewFrom || !stNewTo) return false;

    // 4. Kiểm tra tàu có tồn tại không
    TrainNode* train = trainList->findTrainByCode(trainCode);
    if (!train) return false;

    // 5. Kiểm tra toa xe và giới hạn ghế có hợp lệ không
    int maxSeats = train->getCarriageSeats(carriageNo);
    if (maxSeats == 0 || seatNo <= 0 || seatNo > maxSeats) return false;

    // 6. Kiểm tra lịch trình: Tàu phải đi qua ga khởi hành TRƯỚC ga đến
    bool foundFrom = false;
    bool validDirection = false;
    StopScheduleNode* stop = train->getHeadSchedule();
    
    while (stop != nullptr) {
        if (stop->getStationID() == newFromID) {
            foundFrom = true;
        } else if (stop->getStationID() == newToID && foundFrom) {
            validDirection = true;
            break;
        }
        stop = stop->getNextStop();
    }
    if (!validDirection) return false;

    // 7. Kiểm tra giao thoa khoảng trạm theo mốc Km ga (Segment Booking)
    double newKmStart = min(stNewFrom->getKmMarker(), stNewTo->getKmMarker());
    double newKmEnd = max(stNewFrom->getKmMarker(), stNewTo->getKmMarker());

    TicketNode* curr = head;
    while (curr != nullptr) {
        if (curr->getTrainCode() == trainCode &&
            curr->getCarriageNo() == carriageNo &&
            curr->getSeatNo() == seatNo) 
        {
            StationNode* stExistFrom = stationList->findStationByID(curr->getFromStationID());
            StationNode* stExistTo = stationList->findStationByID(curr->getToStationID());

            if (stExistFrom && stExistTo) {
                double existKmStart = min(stExistFrom->getKmMarker(), stExistTo->getKmMarker());
                double existKmEnd = max(stExistFrom->getKmMarker(), stExistTo->getKmMarker());

                // Hai đoạn [A1, B1] và [A2, B2] giao thoa khi: max(A1, A2) < min(B1, B2)
                double overlapStart = max(newKmStart, existKmStart);
                double overlapEnd = min(newKmEnd, existKmEnd);

                if (overlapStart < overlapEnd) {
                    return false; // Ghế đã được đặt trong phân đoạn này
                }
            }
        }
        curr = curr->getNextTicket();
    }
    
    return true; // Hợp lệ và ghế còn trống
}