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

void TicketList::displayAllTickets() const {
    cout << "\n=== DANH SACH VE DA DAT ===\n";
    TicketNode* temp = head;
    if (!temp) {
        cout << "(Chua co ve nao trong he thong)\n";
        return;
    }
    while (temp != nullptr) {
        cout << "Ma Ve: " << temp->getTicketID()
             << " | Tau: " << temp->getTrainCode()
             << " | Toa: " << temp->getCarriageNo()
             << " | Ghe: " << temp->getSeatNo()
             << " | Chang: Ga " << temp->getFromStationID() 
             << " -> Ga " << temp->getToStationID() << "\n";
        temp = temp->getNextTicket();
    }
}

// ==================== SEGMENT BOOKING (KIỂM TRA GIAO THOA KM) ====================
bool TicketList::isSeatAvailable(StationList* stationList, const string& trainCode, 
                                int carriageNo, int seatNo, int newFromID, int newToID) const 
{
    if (!stationList) return false;

    StationNode* stNewFrom = stationList->findStationByID(newFromID);
    StationNode* stNewTo = stationList->findStationByID(newToID);
    if (!stNewFrom || !stNewTo) return false;

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
                    return false; // Đã bị chiếm trong đoạn này
                }
            }
        }
        curr = curr->getNextTicket();
    }
    return true; // Ghế còn trống
}

void TicketList::displaySeatMap(StationList* stationList, const string& trainCode,
                                int carriageNo, int totalSeats, int fromID, int toID) const 
{
    cout << "\n--- SO DO GHE TOA " << carriageNo << " (TAU " << trainCode << ") ---";
    cout << "\n[Chang: Ga " << fromID << " -> Ga " << toID << "]\n";

    for (int i = 1; i <= totalSeats; ++i) {
        bool free = isSeatAvailable(stationList, trainCode, carriageNo, i, fromID, toID);
        if (free) {
            cout << "[" << i << ": O] "; // O = Trống
        } else {
            cout << "[" << i << ": X] "; // X = Đã đặt
        }
        if (i % 4 == 0) cout << "\n";
    }
    cout << "\n(Ghi chu: O = Con trong, X = Da duoc dat)\n";
}

void TicketList::bookTicketUI(StationList* stationList, TrainList* trainList) {
    string tID, trCode;
    int cNo, sNo, fromID, toID;

    cout << "\n=== CHUONG TRINH DAT VE MOI (SEGMENT BOOKING) ===\n";
    cout << "Nhap Ma Ve: "; cin >> tID;
    cout << "Nhap Ma Tau: "; cin >> trCode;
    cout << "Nhap So Toa: "; cin >> cNo;
    cout << "Nhap ID Ga Di: "; cin >> fromID;
    cout << "Nhap ID Ga Den: "; cin >> toID;

    TrainNode* train = trainList->findTrainByCode(trCode);
    if (!train) {
        cout << "-> [LOI] Khong tim thay tau " << trCode << "\n";
        return;
    }

    int totalSeats = train->getCarriageSeats(cNo);
    if (totalSeats == 0) {
        cout << "-> [LOI] Khong tim thay toa " << cNo << " tren tau " << trCode << "\n";
        return;
    }

    displaySeatMap(stationList, trCode, cNo, totalSeats, fromID, toID);

    cout << "Nhap So Ghe muon dat: "; cin >> sNo;

    if (isSeatAvailable(stationList, trCode, cNo, sNo, fromID, toID)) {
        addTicket(tID, trCode, cNo, sNo, fromID, toID);
        cout << "-> [THANH CONG] Dat ve " << tID << " thanh cong!\n";
    } else {
        cout << "-> [THAT BAI] Ghe so " << sNo << " da co nguoi dat trong chang nay!\n";
    }
}
