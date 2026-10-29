#include "GuiMenu.h"
#include "StationList.h"
#include "TrainList.h"
#include "TicketList.h"
#include "FileManager.h"
#include <iostream>

using namespace std;

int main() {
    StationList stationList;
    TrainList trainList;
    TicketList ticketList;

    cout << "--> Dang khoi tao va nap du lieu ban dau tu file text...\n";

    // 1. Nạp dữ liệu từ các tập tin .txt
    FileManager::loadStations("stations.txt", stationList);
    FileManager::loadTrains("trains.txt", trainList);
    FileManager::loadTickets("tickets.txt", ticketList);

    // 2. BỔ SUNG: TỰ ĐỘNG QUÉT & KHỦNG XUNG ĐỘT NGAY KHI KHỞI ĐỘNG
    trainList.resolveConflictAndScheduleSiding(&stationList);
    
    // Lưu lại lịch trình đã tự động khử xung đột ra file
    FileManager::saveTrains("trains.txt", trainList);

    cout << "--> Nap va khu xung dot thanh cong! Dang mo SFML Dashboard...\n";

    // 3. Chạy giao diện đồ họa GUI
    GuiMenu::runGUIMenu(stationList, trainList, ticketList);

    // 4. Lưu lại dữ liệu an toàn trước khi thoát
    FileManager::saveStations("stations.txt", stationList);
    FileManager::saveTrains("trains.txt", trainList);
    FileManager::saveTickets("tickets.txt", ticketList);

    cout << "Chuong trinh da thoat an toan.\n";
    return 0;
}
