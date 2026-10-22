#include "GuiMenu.h"
#include "StationList.h"
#include "TrainList.h"
#include "TicketList.h"
#include "FileManager.h"
#include "GraphicalSimulationEngine.h"

#include <iostream>
#include <sstream>

using namespace std;

void GuiMenu::runGUIMenu(StationList& stationList, TrainList& trainList, TicketList& ticketList) {
    sf::RenderWindow window(sf::VideoMode(1280, 720), 
                            "PBL2: INTERACTIVE GUI MENU - HE THONG DIEU PHOI TAU HOA", 
                            sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        cout << "[Loi] Khong tim thay font 'arial.ttf'!\n";
    }

    // Danh sách các Nút bấm trên Menu GUI
    vector<GuiButton> buttons;
    float startX = 50.0f;
    float startY = 100.0f;
    float btnW = 320.0f;
    float btnH = 45.0f;
    float gapY = 15.0f;

    // Nhóm 1: Quản lý Ga & Tàu
    buttons.push_back(GuiButton(startX, startY + 0 * (btnH + gapY), btnW, btnH, "1. Danh Sach Tat Ca Cac Ga", font, 1));
    buttons.push_back(GuiButton(startX, startY + 1 * (btnH + gapY), btnW, btnH, "2. Danh Sach Cac Chuyen Tau", font, 2));
    buttons.push_back(GuiButton(startX, startY + 2 * (btnH + gapY), btnW, btnH, "3. Tra Cuu Chi Tiet Tau & Toa Xe", font, 3));

    // Nhóm 2: Nghiệp vụ Vé & Đặt chỗ
    buttons.push_back(GuiButton(startX, startY + 3 * (btnH + gapY), btnW, btnH, "4. Dat Ve & Xem So Do Ghe Chang", font, 4));
    buttons.push_back(GuiButton(startX, startY + 4 * (btnH + gapY), btnW, btnH, "5. Danh Sach Ve Da Ban", font, 5));

    // Nhóm 3: Trực quan hóa & Hệ thống
    buttons.push_back(GuiButton(startX, startY + 5 * (btnH + gapY), btnW, btnH, "6. CAI DAT MO PHONG SFML 2D", font, 6));
    buttons.push_back(GuiButton(startX, startY + 6 * (btnH + gapY), btnW, btnH, "7. Tai Lai Du Lieu Tu File Text", font, 7));
    buttons.push_back(GuiButton(startX, startY + 7 * (btnH + gapY), btnW, btnH, "0. Thoat Chuong Trinh", font, 0));

    // Khung bảng hiển thị dữ liệu bên phải (Data Output Panel)
    sf::RectangleShape outputBox(sf::Vector2f(820.0f, 580.0f));
    outputBox.setPosition(410.0f, 100.0f);
    outputBox.setFillColor(sf::Color(30, 30, 35));
    outputBox.setOutlineThickness(2.0f);
    outputBox.setOutlineColor(sf::Color(100, 100, 100));

    string outputLog = "WELCOME TO PBL2 GRAPHICAL INTERFACE!\nNhấp vao cac nut chuc nang ben trai de tuong tac...";

    while (window.isOpen()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        bool mouseClicked = false;

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                mouseClicked = true;
            }
        }

        // Cập nhật trạng thái rê chuột (Hover) cho tất cả các nút
        for (auto& btn : buttons) {
            btn.updateHover(mousePos);

            // Xử lý Sự kiện khi Click Chuột
            if (btn.isClicked(mousePos, mouseClicked)) {
                switch (btn.actionID) {
                case 1: {
                    outputLog = "=== DANH SACH CAC GA TREN TUYEN ===\n";
                    StationNode* st = stationList.getHead();
                    while (st != nullptr) {
                        outputLog += "ID: " + to_string(st->getStationID()) + 
                                     " | Ten Ga: " + st->getName() + 
                                     " | Km: " + to_string((int)st->getKmMarker()) + 
                                     " | So Ray Phu: " + to_string(st->getNumberOfTracks()) + "\n";
                        st = st->getNextStation();
                    }
                    break;
                }
                case 2: {
                    outputLog = "=== DANH SACH CAC CHUYEN TAU ===\n";
                    TrainNode* tr = trainList.getHead();
                    while (tr != nullptr) {
                        outputLog += "Ma Tau: " + tr->getTrainCode() + 
                                     " | Huong Chay: " + (tr->getDirection() == 1 ? "Nam (Di vao)" : "Bac (Di ra)") + "\n";
                        tr = tr->getNextTrain();
                    }
                    break;
                }
                case 3: {
                    outputLog = "Ban co the tra cuu ma tau truc tiep tren cua so Console đi kem...";
                    break;
                }
                case 4: {
                    // Mở luồng tương tác Đặt vé & Sơ đồ ghế
                    ticketList.bookTicketUI(&stationList, &trainList);
                    outputLog = "Da cap nhat xong giao dich dat ve!";
                    break;
                }
                case 5: {
                    outputLog = "=== DANH SACH VE DA DAT TRONG HE THONG ===\n";
                    TicketNode* tk = ticketList.getHead();
                    while (tk != nullptr) {
                        outputLog += "Ma Ve: " + tk->getTicketID() + 
                                     " | Tau: " + tk->getTrainCode() + 
                                     " | Toa: " + to_string(tk->getCarriageNo()) + 
                                     " | Ghe: " + to_string(tk->getSeatNo()) + 
                                     " | Chang: " + to_string(tk->getFromStationID()) + " -> " + to_string(tk->getToStationID()) + "\n";
                        tk = tk->getNextTicket();
                    }
                    break;
                }
                case 6: {
                    // Mở cửa sổ Mô phỏng SFML 2D
                    GraphicalSimulationEngine::runSFMLSimulation(&stationList, &trainList);
                    break;
                }
                case 7: {
                    FileManager::loadStations("stations.txt", stationList);
                    FileManager::loadTrains("trains.txt", trainList);
                    FileManager::loadTickets("tickets.txt", ticketList);
                    outputLog = "-> Da tai lai du lieu tu file text thanh cong!";
                    break;
                }
                case 0:
                    window.close();
                    break;
                }
            }
        }

        // Render toàn bộ giao diện
        window.clear(sf::Color(24, 25, 28));

        // Tiêu đề Dashboard
        sf::Text headerText("PBL2: DIEU PHOI TAU HOA - BẢNG ĐIỀU KHIỂN GUI", font, 22);
        headerText.setFillColor(sf::Color(241, 196, 15));
        headerText.setPosition(startX, 30.0f);
        window.draw(headerText);

        // Vẽ các nút Menu
        for (auto& btn : buttons) {
            btn.draw(window);
        }

        // Vẽ bảng hiển thị dữ liệu bên phải
        window.draw(outputBox);

        sf::Text logText(outputLog, font, 15);
        logText.setFillColor(sf::Color::White);
        logText.setPosition(425.0f, 115.0f);
        window.draw(logText);

        window.display();
    }
}
