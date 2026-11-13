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
    // Lệnh dữ liệu và khai báo bảo trì được áp dụng trước khi điều phối.
    string commandLog;
    if (FileManager::processInputCommands("data/commands.txt", stationList, trainList, commandLog)) {
        cout << "--> Ket qua commands.txt:\n" << commandLog;
    } else {
        cout << "--> [Canh bao] " << commandLog << "\n";
        trainList.resolveConflictAndScheduleSiding(&stationList);
    }

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
