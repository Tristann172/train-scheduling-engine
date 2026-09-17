#include <iostream>
#include "StationList.h"
#include "TrainList.h"
#include "FileManager.h"

void showMenu() {
    std::cout << "\n============================================\n";
    std::cout << " HE THONG QUAN LY LICH TRINH TAU HOA (PBL2) \n";
    std::cout << "============================================\n";
    std::cout << "1. Xem danh sach cac ga\n";
    std::cout << "2. Tra cuu thong tin chuyen tau\n";
    std::cout << "0. Thoat chuong trinh\n";
    std::cout << "Lua chon cua ban: ";
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
        std::cin >> choice;

        if (choice == 1) {
            stationList.displayAll();
        } 
        else if (choice == 2) {
            std::string code;
            std::cout << "Nhap ma hieu tau (vi du: SE1): ";
            std::cin >> code;
            TrainNode* train = trainList.findTrainByCode(code);
            if (train) {
                std::cout << "\n-> Tim thay tau " << train->getTrainCode() 
                          << " (Huong: " << (train->getDirection() == 1 ? "Nam" : "Bac") << ")\n";
            } else {
                std::cout << "\n-> Khong tim thay tau!\n";
            }
        }
    }

    std::cout << "Da thoat chuong trinh an toan.\n";
    return 0;
}
