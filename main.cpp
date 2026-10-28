#include "GuiMenu.h"
#include "StationList.h"
#include "TrainList.h"
#include "TicketList.h"
#include "FileManager.h"
#include <iostream>

using namespace std;

int main() {
    // 1. Khởi tạo các danh sách con trỏ tự quản lý bộ nhớ Heap
    StationList stationList;
    TrainList trainList;
    TicketList ticketList;

    cout << "--> Dang khoi tao va nap du lieu ban dau tu file text...\n";

    // 2. Nạp dữ liệu tự động từ các file persistence
    FileManager::loadStations("stations.txt", stationList);
    FileManager::loadTrains("trains.txt", trainList);
    FileManager::loadTickets("tickets.txt", ticketList);

    cout << "--> Nap du lieu thanh cong! Dang mo Cua so Do hoa SFML Dashboard...\n";

    // 3. KHỞI CHẠY TRỰC TIẾP CỬA SỔ GUI ĐỒ HỌA TƯƠNG TÁC BẰNG CHUỘT
    GuiMenu::runGUIMenu(stationList, trainList, ticketList);

    // 4. Tự động lưu toàn bộ dữ liệu ra tập tin text trước khi kết thúc
    FileManager::saveStations("stations.txt", stationList);
    FileManager::saveTrains("trains.txt", trainList);
    FileManager::saveTickets("tickets.txt", ticketList);

    cout << "Chuong trinh da thoat an toan (Zero Memory Leaks).\n";
    return 0;
}
