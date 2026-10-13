#include <iostream>
#include <string>
#include <cstdlib>

#include "StationList.h"
#include "TrainList.h"
#include "TicketList.h"

#include "FileManager.h"


#include "StationNode.h"
#include "TrainNode.h"
#include "CarriageNode.h"
#include "StopScheduleNode.h"

using namespace std;

void displayMenu() {
    cout << "\n=======================================================\n";
    cout << "   HE THONG QUAN LY LICH TRINH & DIEU PHOI TAU HOA    \n";
    cout << "=======================================================\n";
    cout << " [ QUAN LY DANH MUC GA ]                               \n";
    cout << " 1. Hien thi danh sach tat ca cac ga                   \n";
    cout << " 2. Them ga moi vao tuyen                              \n";
    cout << " 3. Tim kiem ga theo ID                                \n";
    cout << "-------------------------------------------------------\n";
    cout << " [ QUAN LY DOAN TAU & LICH TRINH ]                    \n";
    cout << " 4. Hien thi danh sach cac chuyen tau                  \n";
    cout << " 5. Tra cuu chi tiet tau (Toa xe & Lich dung do)      \n";
    cout << " 6. Them chuyen tau moi                                \n";
    cout << "-------------------------------------------------------\n";
    cout << " [ NGHIEP VU VE & CHANG ]                              \n";
    cout << " 7. Tra cuu so do ghe & Dat ve phan doan chang         \n";
    cout << " 8. Hien thi danh sach tat ca ve da dat                \n";
    cout << " 9. Huy ve va cap nhat tra lai ghe trong               \n";
    cout << "-------------------------------------------------------\n";
    cout << " [ HE THONG & DU LIEU ]                                \n";
    cout << " 10. Tai lai (Reload) du lieu tu file text             \n";
    cout << " 0. Thoat chuong trinh                                 \n";
    cout << "=======================================================\n";
    cout << "Nhap lua chon cua ban: ";
}


void displayTrainDetails(TrainNode* train) {
    if (!train) return;

    cout << "\n--------------------------------------------------\n";
    cout << "MA TAU: " << train->getTrainCode() 
         << " | HUONG CHAY: " << (train->getDirection() == 1 ? "Nam (Di vao)" : "Bac (Di ra)") << "\n";

    cout << "  [+] DANH SACH TOA XE:\n";
    CarriageNode* carriage = train->getHeadCarriage();
    if (!carriage) {
        cout << "      (Chua co toa xe)\n";
    }
    while (carriage != nullptr) {
        cout << "      - Toa " << carriage->getCarriageNo() 
             << " | Loai: " << carriage->getType() 
             << " | So ghe: " << carriage->getTotalSeats() << "\n";
        carriage = carriage->getNextCarriage();
    }

    cout << "  [+] LICH TRINH DUNG DO TAI CAC GA:\n";
    StopScheduleNode* stop = train->getHeadSchedule();
    if (!stop) {
        cout << "      (Chua co lich dung do)\n";
    }
    while (stop != nullptr) {
        cout << "      - Ga ID: " << stop->getStationID() 
             << " | Gio den: " << stop->getArrivalMinute() << "' "
             << "| Gio đi: " << stop->getDepartureMinute() << "' "
             << "| Ray dung: " << stop->getTrackUsed() << "\n";
        stop = stop->getNextStop();
    }
}

int main() {
    StationList stationList;
    TrainList trainList;
    TicketList ticketList;

    cout << "--> Dang khoi tao va nap du lieu tu file text...\n";

    FileManager::loadStations("stations.txt", stationList);
    FileManager::loadTrains("trains.txt", trainList);
    FileManager::loadTickets("tickets.txt", ticketList);

    cout << "--> Nap du lieu thanh cong!\n";

    int choice = -1;
    while (choice != 0) {
        displayMenu();
        if (!(cin >> choice)) {
            cout << "\n[Loi] Lua chon khong hop le! Vui long nhap so.\n";
            cin.clear();
            cin.ignore(10000, '\n');
            continue;
        }

        switch (choice) {

        case 1: {
            stationList.displayAll();
            break;
        }

        case 2: {
            int id, tracks;
            string name;
            double km;

            cout << "\n--- THEM GA MOI ---\n";
            cout << "Nhap ID ga: "; cin >> id;
            cin.ignore();
            cout << "Nhap ten ga: "; getline(cin, name);
            cout << "Nhap moc km: "; cin >> km;
            cout << "Nhap so luong ray phu (duong tranh): "; cin >> tracks;

            stationList.addStation(id, name, km, tracks);
            FileManager::saveStations("stations.txt", stationList);
            cout << "-> Da them ga " << name << " va luu file thanh cong!\n";
            break;
        }

        case 3: {
            int id;
            cout << "\nNhap ID ga can tim: "; cin >> id;
            StationNode* st = stationList.findStationByID(id);
            if (st) {
                cout << "-> TIM THAY GA: ID " << st->getStationID() 
                     << " | Ten: " << st->getName() 
                     << " | Km: " << st->getKmMarker() 
                     << " | So ray phu: " << st->getNumberOfTracks() << "\n";
            } else {
                cout << "-> Khong tim thay ga co ID: " << id << "\n";
            }
            break;
        }

        case 4: {
            cout << "\n=== DANH SACH CAC CHUYEN TAU TRONG HE THONG ===\n";
            TrainNode* temp = trainList.getHead();
            if (!temp) {
                cout << "(Chua co du lieu tau hoa)\n";
            }
            while (temp != nullptr) {
                cout << "- Ma tau: " << temp->getTrainCode() 
                     << " | Huong: " << (temp->getDirection() == 1 ? "Nam (Di vao)" : "Bac (Di ra)") << "\n";
                temp = temp->getNextTrain();
            }
            break;
        }

        case 5: {
            string code;
            cout << "\nNhap ma hieu tau (vi du: SE1): ";
            cin >> code;
            TrainNode* train = trainList.findTrainByCode(code);
            if (train) {
                displayTrainDetails(train);
            } else {
                cout << "-> Khong tim thay tau co ma: " << code << "\n";
            }
            break;
        }

        case 6: {
            string code;
            int dir;
            cout << "\n--- THEM CHUYEN TAU MOI ---\n";
            cout << "Nhap ma hieu tau: "; cin >> code;
            cout << "Nhap huong (1: Nam, 0: Bac): "; cin >> dir;

            trainList.addTrain(code, dir);
            FileManager::saveTrains("trains.txt", trainList);
            cout << "-> Da them tau " << code << " va luu file thanh cong!\n";
            break;
        }

        case 7: {
            ticketList.bookTicketUI(&stationList, &trainList);
            break;
        }

        case 8: {
            ticketList.displayAllTickets();
            break;
        }

        case 9: {
            string tID;
            cout << "\n--- HUY VE THONG TIN DAT CHO ---\n";
            cout << "Nhap ma ve can huy (vi du: TCK001): ";
            cin >> tID;

            if (ticketList.cancelTicket(tID)) {
                FileManager::saveTickets("tickets.txt", ticketList);
                cout << "-> Huy ve " << tID << " thanh cong va da tra lai ghe trong!\n";
            } else {
                cout << "-> [Loi] Khong tim thay ma ve " << tID << " trong he thong!\n";
            }
            break;
        }

        case 10: {
            cout << "\nDang tai lai du lieu tu file text...\n";
            FileManager::loadStations("stations.txt", stationList);
            FileManager::loadTrains("trains.txt", trainList);
            FileManager::loadTickets("tickets.txt", ticketList);
            cout << "-> Nap lai du lieu thanh cong!\n";
            break;
        }

        case 0: {
            cout << "\n[!] Dang thoat chuong trinh va thu hoi bo nho Heap...\n";
            break;
        }

        default:
            cout << "\n[Loi] Lua chon khong hop le!\n";
            break;
        }
    }

    FileManager::saveStations("stations.txt", stationList);
    FileManager::saveTrains("trains.txt", trainList);
    FileManager::saveTickets("tickets.txt", ticketList);

    cout << "Chuong trinh da thoat an toan (Zero Leaks).\n";
    return 0;
}
