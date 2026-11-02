#include "TicketList.h"
#include "StationList.h"
#include "TrainList.h"
#include "StationNode.h"
#include "TrainNode.h"
#include "StopScheduleNode.h"
#include <iostream>
#include <algorithm>

using namespace std;

TicketList::TicketList() : head(nullptr), tail(nullptr) {}

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
    tail = nullptr;
}

// Chèn vé mới vào cuối danh sách liên kết với độ phức tạp O(1)
void TicketList::addTicket(const string& ticketID, const string& trainCode, 
                           int carriageNo, int seatNo, int fromID, int toID) {
    TicketNode* newNode = new TicketNode(ticketID, trainCode, carriageNo, seatNo, fromID, toID);
    if (!head) {
        head = tail = newNode;
        return;
    }
    tail->setNextTicket(newNode);
    tail = newNode;
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

    // Trường hợp vé cần hủy nằm ngay đầu danh sách
    if (head->getTicketID() == ticketID) {
        TicketNode* temp = head;
        head = head->getNextTicket();
        if (!head) tail = nullptr; // Danh sách trở thành rỗng
        delete temp;
        return true;
    }

    TicketNode* curr = head;
    while (curr->getNextTicket() && curr->getNextTicket()->getTicketID() != ticketID) {
        curr = curr->getNextTicket();
    }

    // Tìm thấy vé ở giữa hoặc cuối danh sách
    if (curr->getNextTicket()) {
        TicketNode* temp = curr->getNextTicket();
        curr->setNextTicket(temp->getNextTicket());
        if (temp == tail) {
            tail = curr; // Cập nhật lại tail nếu hủy vé cuối cùng
        }
        delete temp;
        return true;
    }
    return false;
}

bool TicketList::checkCarriageSeats(StationList* stationList, TrainList* trainList,
                                    const string& trainCode, int carriageNo,
                                    int newFromID, int newToID,
                                    bool seatFreeOut[], int maxSeats) const 
{
    if (!stationList || !trainList || newFromID == newToID || maxSeats <= 0) return false;

    StationNode* stNewFrom = stationList->findStationByID(newFromID);
    StationNode* stNewTo   = stationList->findStationByID(newToID);
    if (!stNewFrom || !stNewTo) return false;

    TrainNode* train = trainList->findTrainByCode(trainCode);
    if (!train) return false;

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

    if (!validDirection) {
        return false; 
    }

    for (int i = 1; i <= maxSeats; ++i) {
        seatFreeOut[i] = true;
    }

    double newKmStart = min(stNewFrom->getKmMarker(), stNewTo->getKmMarker());
    double newKmEnd   = max(stNewFrom->getKmMarker(), stNewTo->getKmMarker());

    TicketNode* curr = head;
    while (curr != nullptr) {
        int sNo = curr->getSeatNo();

        if (curr->getTrainCode() == trainCode &&
            curr->getCarriageNo() == carriageNo &&
            sNo >= 1 && sNo <= maxSeats && seatFreeOut[sNo]) 
        {
            StationNode* stExistFrom = stationList->findStationByID(curr->getFromStationID());
            StationNode* stExistTo   = stationList->findStationByID(curr->getToStationID());

            if (stExistFrom && stExistTo) {
                double existKmStart = min(stExistFrom->getKmMarker(), stExistTo->getKmMarker());
                double existKmEnd   = max(stExistFrom->getKmMarker(), stExistTo->getKmMarker());

                if (max(newKmStart, existKmStart) < min(newKmEnd, existKmEnd)) {
                    seatFreeOut[sNo] = false;
                }
            }
        }
        curr = curr->getNextTicket();
    }

    return true;
}

// ==================== KIỂM TRA ĐƠN LẺ KHI ĐẶT VÉ (SEGMENT BOOKING) ====================
bool TicketList::isSeatAvailable(StationList* stationList, TrainList* trainList, const string& trainCode, 
                                 int carriageNo, int seatNo, int newFromID, int newToID) const 
{
    if (!stationList || !trainList || newFromID == newToID) return false;

    StationNode* stNewFrom = stationList->findStationByID(newFromID);
    StationNode* stNewTo   = stationList->findStationByID(newToID);
    if (!stNewFrom || !stNewTo) return false;

    TrainNode* train = trainList->findTrainByCode(trainCode);
    if (!train) return false;

    int maxSeats = train->getCarriageSeats(carriageNo);
    if (maxSeats == 0 || seatNo <= 0 || seatNo > maxSeats) return false;

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

    double newKmStart = min(stNewFrom->getKmMarker(), stNewTo->getKmMarker());
    double newKmEnd   = max(stNewFrom->getKmMarker(), stNewTo->getKmMarker());

    TicketNode* curr = head;
    while (curr != nullptr) {
        if (curr->getTrainCode() == trainCode &&
            curr->getCarriageNo() == carriageNo &&
            curr->getSeatNo() == seatNo) 
        {
            StationNode* stExistFrom = stationList->findStationByID(curr->getFromStationID());
            StationNode* stExistTo   = stationList->findStationByID(curr->getToStationID());

            if (stExistFrom && stExistTo) {
                double existKmStart = min(stExistFrom->getKmMarker(), stExistTo->getKmMarker());
                double existKmEnd   = max(stExistFrom->getKmMarker(), stExistTo->getKmMarker());

                if (max(newKmStart, existKmStart) < min(newKmEnd, existKmEnd)) {
                    return false;
                }
            }
        }
        curr = curr->getNextTicket();
    }
    return true;
}