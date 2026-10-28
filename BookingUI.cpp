#include "BookingUI.h"
#include "FileManager.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>

using namespace std;

void BookingUI::displayAllTickets(const TicketList& ticketList) {
    cout << "\n=== DANH SACH VE DA DAT TRONG HE THONG ===\n";
    TicketNode* current = ticketList.getHead();
    
    if (current == nullptr) {
        cout << "(Chua co ve nao duoc dat)\n";
        return;
    }
    while (current != nullptr) {
        cout << "Ma ve: " << current->getTicketID()
             << " | Tau: " << current->getTrainCode()
             << " | Toa: " << current->getCarriageNo()
             << " | Ghe: " << current->getSeatNo()
             << " | Chang: Ga " << current->getFromStationID() 
             << " -> Ga " << current->getToStationID() << "\n";
        current = current->getNextTicket();
    }
}

void BookingUI::displaySeatMap(const TicketList& ticketList, StationList* stationList, 
                               const std::string& trainCode, int carriageNo, 
                               int totalSeats, int fromID, int toID) {
    cout << "\n-------------------------------------------------------------\n";
    cout << "   SO DO GHE TOA " << carriageNo << " (TAU " << trainCode 
         << ") CHUYEN GA " << fromID << " -> " << toID << "\n";
    cout << "   Chu thich: [XX] = Da ban | [01] = Ghe con trong\n";
    cout << "-------------------------------------------------------------\n";

    for (int i = 1; i <= totalSeats; ++i) {
        // Gọi hàm kiểm tra qua ticketList
        bool available = ticketList.isSeatAvailable(stationList, trainCode, carriageNo, i, fromID, toID);
        
        if (available) {
            cout << "[" << setfill('0') << setw(2) << i << "] ";
        } else {
            cout << "[ XX ] ";
        }

        if (i % 4 == 0) {
            cout << "\n";
        } else if (i % 2 == 0) {
            cout << "   "; 
        }
    }
    cout << "-------------------------------------------------------------\n";
}

void BookingUI::bookTicketUI(TicketList& ticketList, StationList* stationList, TrainList* trainList) {
    if (stationList == nullptr || trainList == nullptr) {
        cout << "[Loi] Du lieu he thong chua san sang!\n";
        return;
    }

    string trainCode;
    int fromID, toID;

    cout << "\n=== TRA CUU & DAT VE PHAN DOAN CHANG ===\n";
    cout << "Nhap ma hieu tau (vi du: SE1): "; cin >> trainCode;

    TrainNode* train = trainList->findTrainByCode(trainCode);
    if (train == nullptr) {
        cout << "[Loi] Khong tim thay tau co ma " << trainCode << "!\n";
        return;
    }

    cout << "Nhap ID Ga di: "; cin >> fromID;
    cout << "Nhap ID Ga den: "; cin >> toID;

    if (fromID == toID) {
        cout << "[Loi] Ga di va ga den khong duoc trung nhau!\n";
        return;
    }

    StationNode* stFrom = stationList->findStationByID(fromID);
    StationNode* stTo = stationList->findStationByID(toID);

    if (stFrom == nullptr || stTo == nullptr) {
        cout << "[Loi] Ma ga di hoac ga den khong ton tai tren he thong!\n";
        return;
    }

    StopScheduleNode* sched = train->getHeadSchedule();
    StopScheduleNode* stopFrom = nullptr;
    StopScheduleNode* stopTo = nullptr;

    while (sched != nullptr) {
        if (sched->getStationID() == fromID && stopFrom == nullptr) {
            stopFrom = sched;
        } else if (sched->getStationID() == toID && stopFrom != nullptr) {
            stopTo = sched;
            break; 
        }
        sched = sched->getNextStop();
    }

    if (stopFrom == nullptr || stopTo == nullptr) {
        cout << "[Loi] Tau " << trainCode << " khong co lich trinh chay tu ga " 
             << fromID << " den ga " << toID << "!\n";
        return;
    }

    CarriageNode* currCarriage = train->getHeadCarriage();
    if (currCarriage == nullptr) {
        cout << "[Loi] Tau chua duoc thiet lap danh sach toa xe!\n";
        return;
    }

    cout << "\nDanh sach toa xe hien co tren tau " << trainCode << ":\n";
    CarriageNode* tempCarriage = currCarriage;
    while (tempCarriage != nullptr) {
        cout << " + Toa " << tempCarriage->getCarriageNo() 
             << " | Loai: " << tempCarriage->getType() 
             << " | Tong ghe: " << tempCarriage->getTotalSeats() << "\n";
        tempCarriage = tempCarriage->getNextCarriage();
    }

    int selectedCarriage;
    cout << "Nhap so toa chon: "; cin >> selectedCarriage;

    currCarriage = train->getHeadCarriage();
    while (currCarriage != nullptr && currCarriage->getCarriageNo() != selectedCarriage) {
        currCarriage = currCarriage->getNextCarriage();
    }

    if (currCarriage == nullptr) {
        cout << "[Loi] So toa khong hop le!\n";
        return;
    }

    displaySeatMap(ticketList, stationList, trainCode, selectedCarriage, currCarriage->getTotalSeats(), fromID, toID);

    int selectedSeat;
    cout << "Nhap so ghe ban muon dat: "; cin >> selectedSeat;

    if (selectedSeat < 1 || selectedSeat > currCarriage->getTotalSeats()) {
        cout << "[Loi] So ghe nam ngoai pham vi toa!\n";
        return;
    }

    if (ticketList.isSeatAvailable(stationList, trainCode, selectedCarriage, selectedSeat, fromID, toID)) {
        string ticketID;
        do {
            ticketID = "TCK" + to_string(rand() % 90000 + 10000);
        } while (ticketList.findTicketByID(ticketID) != nullptr);

        ticketList.addTicket(ticketID, trainCode, selectedCarriage, selectedSeat, fromID, toID);
        FileManager::saveTickets("tickets.txt", ticketList);

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