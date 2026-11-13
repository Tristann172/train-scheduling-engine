#include "GuiMenu.h"
#include "StationList.h"
#include "TrainList.h"
#include "FileManager.h"
#include <iostream>

using namespace std;

int main() {
    StationList stationList;
    TrainList trainList;
    cout << "--> Dang khoi tao va nap du lieu ban dau tu file text...\n";

    // 1. Nạp dữ liệu từ các tập tin .txt
    FileManager::loadStations("data/stations.txt", stationList);
    FileManager::loadTrains("data/trains.txt", trainList);
    // Dữ liệu bảo trì được lưu cùng trains.txt và thao tác trực tiếp trên GUI.
    trainList.resolveConflictAndScheduleSiding(&stationList);

    // Lưu lịch trình sau khi đã giải quyết xung đột.
    FileManager::saveTrains("data/trains.txt", trainList);

    cout << "--> Nap va khu xung dot thanh cong! Dang mo SFML Dashboard...\n";

    // 3. Chạy giao diện đồ họa GUI
    GuiMenu::runGUIMenu(stationList, trainList);

    // 4. Lưu lại dữ liệu an toàn trước khi thoát
    FileManager::saveStations("data/stations.txt", stationList);
    FileManager::saveTrains("data/trains.txt", trainList);
    cout << "Chuong trinh da thoat an toan.\n";
    return 0;
}
