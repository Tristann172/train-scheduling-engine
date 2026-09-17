#include <iostream>
#include <string>
#include "StationList.h"
#include "TrainList.h"
#include "FileManager.h"

using namespace std;

void showMenu() {
    cout << "\n============================================\n";
    cout << " HE THONG QUAN LY LICH TRINH TAU HOA (PBL2) \n";
    cout << "============================================\n";
    cout << "1. Xem danh sach cac ga\n";
    cout << "2. Tra cuu thong tin chuyen tau\n";
    cout << "0. Thoat chuong trinh\n";
    cout << "Lua chon cua ban: ";
}

int main() {
    StationList stationList;
    TrainList trainList;

    // Nap du lieu tu file khi khoi dong
    FileManager::loadStations("stations.txt", stationList);
    FileManager::loadTrains("trains.txt", trainList);

    int choice = -1;
    while (choice != 0) {
        showMenu();
        cin >> choice;

        if (choice == 1) {
            stationList.displayAll();
        } 
        else if (choice == 2) {
            string code;
            cout << "Nhap ma hieu tau (vi du: SE1): ";
            cin >> code;
            TrainNode* train = trainList.findTrainByCode(code);
            if (train) {
                cout << "\n-> Tim thay tau " << train->getTrainCode() 
                     << " (Huong: " << (train->getDirection() == 1 ? "Nam" : "Bac") << ")\n";
            } else {
                cout << "\n-> Khong tim thay tau!\n";
            }
        }
    }

    cout << "Da thoat chuong trinh an toan.\n";
    return 0;
}
