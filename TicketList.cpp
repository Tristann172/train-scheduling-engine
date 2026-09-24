#include "TicketList.h"
#include "StationList.h"
#include "TrainList.h"
#include "FileManager.h"
#include <iomanip>
#include <cstdlib>
#include <iostream>

TicketList::TicketList() : head(nullptr) {}

void TicketList::clear() {
    while (head != nullptr) {
        TicketNode* temp = head;
        head = head->getNextTicket();
        delete temp; 
    }
    head = nullptr;
}

TicketList::~TicketList() {
    clear();
}

TicketNode* TicketList::getHead() const {
    return head;
}

void TicketList::addTicket(const std::string& ticketID, const std::string& trainCode,
                           int carriageNo, int seatNo, int fromID, int toID) {
    TicketNode* newNode = new TicketNode(ticketID, trainCode, carriageNo, seatNo, fromID, toID);
    
    if (head == NULL) {
        head = newNode;
    } else {
        TicketNode* current = head;
        while (current->getNextTicket() != NULL) {
            current = current->getNextTicket();
        }
        current->setNextTicket(newNode);
    }
}

bool TicketList::isSeatAvailable(StationList* stationList, const std::string& trainCode,
                                 int carriageNo, int seatNo, int newFromID, int newToID) const {
    
    StationNode* newFromSt = stationList->findStationByID(newFromID);
    StationNode* newToSt = stationList->findStationByID(newToID);

    if (newFromSt == NULL || newToSt == NULL) return false; 

    double newKm1 = newFromSt->getKmMarker();
    double newKm2 = newToSt->getKmMarker();
    double newMinKm = (newKm1 < newKm2) ? newKm1 : newKm2;
    double newMaxKm = (newKm1 > newKm2) ? newKm1 : newKm2;

    TicketNode* current = head;
    while (current != NULL) {
        if (current->getTrainCode() == trainCode &&
            current->getCarriageNo() == carriageNo &&
            current->getSeatNo() == seatNo) {

            StationNode* existFromSt = stationList->findStationByID(current->getFromStationID());
            StationNode* existToSt = stationList->findStationByID(current->getToStationID());

            if (existFromSt != NULL && existToSt != NULL) {
                double existKm1 = existFromSt->getKmMarker();
                double existKm2 = existToSt->getKmMarker();
                double existMinKm = (existKm1 < existKm2) ? existKm1 : existKm2;
                double existMaxKm = (existKm1 > existKm2) ? existKm1 : existKm2;

                if (!(existMaxKm <= newMinKm || existMinKm >= newMaxKm)) {
                    return false; 
                }
            }
        }
        current = current->getNextTicket();
    }
    return true; 
}

TicketNode* TicketList::findTicketByID(const std::string& ticketID) const {
    TicketNode* current = head;
    while (current != NULL) {
        if (current->getTicketID() == ticketID) {
            return current;
        }
        current = current->getNextTicket();
    }
    return NULL;
}

bool TicketList::cancelTicket(const std::string& ticketID) {
    if (head == NULL) return false;
    if (head->getTicketID() == ticketID) {
        TicketNode* temp = head;
        head = head->getNextTicket();
        delete temp; // Thu hồi vùng nhớ Heap
        return true;
    }
    TicketNode* current = head;
    while (current->getNextTicket() != NULL) {
        if (current->getNextTicket()->getTicketID() == ticketID) {
            TicketNode* temp = current->getNextTicket();
            current->setNextTicket(temp->getNextTicket());
            delete temp; // Thu hồi vùng nhớ Heap
            return true;
        }
        current = current->getNextTicket();
    }
    return false;
}

void TicketList::displayAllTickets() const {
    cout << "\n=== DANH SACH VE DA DAT TRONG HE THONG ===\n";
    TicketNode* current = head;
    if (current == NULL) {
        cout << "(Chua co ve nao duoc dat)\n";
        return;
    }
    while (current != NULL) {
        cout << "Ma ve: " << current->getTicketID()
             << " | Tau: " << current->getTrainCode()
             << " | Toa: " << current->getCarriageNo()
             << " | Ghe: " << current->getSeatNo()
             << " | Chang: Ga " << current->getFromStationID() 
             << " -> Ga " << current->getToStationID() << "\n";
        current = current->getNextTicket();
    }
}

void TicketList::displaySeatMap(StationList* stationList, const std::string& trainCode, 
                                int carriageNo, int totalSeats, int fromID, int toID) const 
{
    cout << "\n-------------------------------------------------------------\n";
    cout << "   SO DO GHE TOA " << carriageNo << " (TAU " << trainCode 
         << ") CHUYEN GA " << fromID << " -> " << toID << "\n";
    cout << "   Chu thich: [XX] = Da ban | [01] = Ghe con trong\n";
    cout << "-------------------------------------------------------------\n";

    for (int i = 1; i <= totalSeats; ++i) {
        bool available = isSeatAvailable(stationList, trainCode, carriageNo, i, fromID, toID);
        
        if (available) {
            cout << "[" << setfill('0') << setw(2) << i << "] ";
        } else {
            cout << "[ XX ] ";
        }

        // Tạo lối đi giữa hàng ghế trong toa tàu
        if (i % 4 == 0) {
            cout << "\n";
        } else if (i % 2 == 0) {
            cout << "   "; 
        }
    }
    cout << "-------------------------------------------------------------\n";
}

void TicketList::bookTicketUI(StationList* stationList, TrainList* trainList) {
    if (stationList == NULL || trainList == NULL) {
        cout << "[Loi] Du lieu he thong chua san sang!\n";
        return;
    }

    string trainCode;
    int fromID, toID;

    cout << "\n=== TRA CUU & DAT VE PHAN DOAN CHANG ===\n";
    cout << "Nhap ma hieu tau (vi du: SE1): "; cin >> trainCode;

    TrainNode* train = trainList->findTrainByCode(trainCode);
    if (train == NULL) {
        cout << "[Loi] Khong tim thay tau co ma " << trainCode << "!\n";
        return;
    }

    cout << "Nhap ID Ga di: "; cin >> fromID;
    cout << "Nhap ID Ga den: "; cin >> toID;

    StationNode* stFrom = stationList->findStationByID(fromID);
    StationNode* stTo = stationList->findStationByID(toID);

    if (stFrom == NULL || stTo == NULL) {
        cout << "[Loi] Ma ga di hoac ga den khong ton tai!\n";
        return;
    }

    CarriageNode* currCarriage = train->getHeadCarriage();
    if (currCarriage == NULL) {
        cout << "[Loi] Tau chua duoc thiet lap danh sach toa xe!\n";
        return;
    }

    cout << "\nDanh sach toa xe hien co tren tau " << trainCode << ":\n";
    while (currCarriage != NULL) {
        cout << " + Toa " << currCarriage->getCarriageNo() 
             << " | Loai: " << currCarriage->getType() 
             << " | Tong ghe: " << currCarriage->getTotalSeats() << "\n";
        currCarriage = currCarriage->getNextCarriage();
    }

    int selectedCarriage;
    cout << "Nhap so toa chon: "; cin >> selectedCarriage;

    currCarriage = train->getHeadCarriage();
    while (currCarriage != NULL && currCarriage->getCarriageNo() != selectedCarriage) {
        currCarriage = currCarriage->getNextCarriage();
    }

    if (currCarriage == NULL) {
        cout << "[Loi] So toa khong hop le!\n";
        return;
    }

    displaySeatMap(stationList, trainCode, selectedCarriage, currCarriage->getTotalSeats(), fromID, toID);

    int selectedSeat;
    cout << "Nhap so ghe ban muon dat: "; cin >> selectedSeat;

    if (selectedSeat < 1 || selectedSeat > currCarriage->getTotalSeats()) {
        cout << "[Loi] So ghe nam ngoai pham vi toa!\n";
        return;
    }

    if (isSeatAvailable(stationList, trainCode, selectedCarriage, selectedSeat, fromID, toID)) {
        string ticketID = "TCK" + to_string(rand() % 900 + 100);
        
        addTicket(ticketID, trainCode, selectedCarriage, selectedSeat, fromID, toID);
        
        FileManager::saveTickets("tickets.txt", *this);

        cout << "\n==================================================\n";
        cout << " [THANH CONG] DA CAP VE VOI THONG TIN SAU:        \n";
        cout << "  - Ma ve: " << ticketID << "\n";
        cout << "  - Tau: " << trainCode << " | Toa: " << selectedCarriage << " | Ghe: " << selectedSeat << "\n";
        cout << "  - Chang: " << stFrom->getName() << " -> " << stTo->getName() << "\n";
        cout << "==================================================\n";
    } else {
        cout << "\n[THAT BAI] Ghe " << selectedSeat << " da duoc ban cho khach khac trong phan doan chang nay!\n";
    }
}
