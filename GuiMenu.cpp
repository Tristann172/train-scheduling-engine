#include "GuiMenu.h"
#include "StationList.h"
#include "TrainList.h"
#include "TicketList.h"
#include "FileManager.h"
#include "GraphicalSimulationEngine.h"

#include "StationNode.h"
#include "TrainNode.h"
#include "CarriageNode.h"
#include "StopScheduleNode.h"
#include "TicketNode.h"

#include <iostream>
#include <sstream>

using namespace std;

void GuiMenu::runGUIMenu(StationList& stationList, TrainList& trainList, TicketList& ticketList) {
    sf::RenderWindow window(sf::VideoMode(1280, 720), 
                            "PBL2: HE THONG QUAN LY LICH TRINH & DIEU PHOI TAU HOA (GRAPHICAL DASHBOARD)", 
                            sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        cout << "[Loi] Khong tim thay font 'arial.ttf', chữ có thể không hiển thị đúng.\n";
    }

    // Thiết lập danh sách Nút bấm bên cột Menu trái
    vector<GuiButton> buttons;
    float startX = 40.0f;
    float startY = 100.0f;
    float btnW = 340.0f;
    float btnH = 50.0f;
    float gapY = 15.0f;

    buttons.push_back(GuiButton(startX, startY + 0 * (btnH + gapY), btnW, btnH, "1. DANH SACH DANH MUC GA TREN TUYEN", font, 1));
    buttons.push_back(GuiButton(startX, startY + 1 * (btnH + gapY), btnW, btnH, "2. DANH SACH CAC CHUYEN TAU", font, 2));
    buttons.push_back(GuiButton(startX, startY + 2 * (btnH + gapY), btnW, btnH, "3. TRA CUU LICH TRINH & TOA XE TAU", font, 3));
    buttons.push_back(GuiButton(startX, startY + 3 * (btnH + gapY), btnW, btnH, "4. DANH SACH VE DA DAT TRONG HE THONG", font, 4));
    buttons.push_back(GuiButton(startX, startY + 4 * (btnH + gapY), btnW, btnH, "5. MO CUA SO MO PHONG RUNNING 2D (>>>)", font, 5));
    buttons.push_back(GuiButton(startX, startY + 5 * (btnH + gapY), btnW, btnH, "6. RELOAD DU LIEU TU FILE PERSISTENCE", font, 6));
    buttons.push_back(GuiButton(startX, startY + 6 * (btnH + gapY), btnW, btnH, "0. THOAT CHUONG TRINH", font, 0));

    // Khung hiển thị dữ liệu kết quả bên phải (Data Output Panel)
    sf::RectangleShape outputBox(sf::Vector2f(830.0f, 580.0f));
    outputBox.setPosition(410.0f, 100.0f);
    outputBox.setFillColor(sf::Color(30, 30, 35));
    outputBox.setOutlineThickness(2.0f);
    outputBox.setOutlineColor(sf::Color(100, 100, 100));

    string outputLog = "WELCOME TO PBL2 INTERACTIVE GRAPHICAL DASHBOARD!\n";
    outputLog += "---------------------------------------------------------\n";
    outputLog += "Nhap/Click truc tiep vao cac NUT BAM ben trai de thao tac...\n";

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

        // Cập nhật hiệu ứng Rê chuột & Xử lý Click Chuột
        for (auto& btn : buttons) {
            btn.updateHover(mousePos);

            if (btn.isClicked(mousePos, mouseClicked)) {
                switch (btn.actionID) {
                
                // CLICK NÚT 1: Danh sách Ga
                case 1: {
                    outputLog = "=== DANH SACH CAC GA DONG SAT TREN TUYEN ===\n\n";
                    StationNode* st = stationList.getHead();
                    while (st != nullptr) {
                        outputLog += " [GA] ID: " + to_string(st->getStationID()) + 
                                     " | Ten Ga: " + st->getName() + 
                                     " | Moc Km: " + to_string((int)st->getKmMarker()) + 
                                     " km | So Ray Phu: " + to_string(st->getNumberOfTracks()) + "\n";
                        st = st->getNextStation();
                    }
                    break;
                }

                // CLICK NÚT 2: Danh sách Tàu
                case 2: {
                    outputLog = "=== DANH SACH CAC CHUYEN TAU CHAY TREN TUYEN ===\n\n";
                    TrainNode* tr = trainList.getHead();
                    while (tr != nullptr) {
                        outputLog += " [TAU] Ma Tau: " + tr->getTrainCode() + 
                                     " | Huong Chay: " + (tr->getDirection() == 1 ? "Nam (Di vao)" : "Bac (Di ra)") + "\n";
                        tr = tr->getNextTrain();
                    }
                    break;
                }

                // CLICK NÚT 3: Tra cứu lịch trình chi tiết
                case 3: {
                    outputLog = "=== CHI TIET LICH TRINH & TOA XE CAC TAU ===\n\n";
                    TrainNode* tr = trainList.getHead();
                    while (tr != nullptr) {
                        outputLog += "-> TAU " + tr->getTrainCode() + ":\n";
                        
                        // In Toa xe
                        CarriageNode* c = tr->getHeadCarriage();
                        outputLog += "   + Toa Xe: ";
                        while (c != nullptr) {
                            outputLog += "[Toa " + to_string(c->getCarriageNo()) + "-" + c->getType() + "] ";
                            c = c->getNextCarriage();
                        }
                        outputLog += "\n";

                        // In Lịch đỗ
                        StopScheduleNode* s = tr->getHeadSchedule();
                        outputLog += "   + Lich Do: ";
                        while (s != nullptr) {
                            outputLog += "(Ga " + to_string(s->getStationID()) + ": " + to_string(s->getArrivalMinute()) + "'->" + to_string(s->getDepartureMinute()) + "') ";
                            s = s->getNextStop();
                        }
                        outputLog += "\n---------------------------------------------------------\n";
                        tr = tr->getNextTrain();
                    }
                    break;
                }

                // CLICK NÚT 4: Danh sách Vé đã bán
                case 4: {
                    outputLog = "=== DANH SACH VE DA PHAT HANH TRONG HE THONG ===\n\n";
                    TicketNode* tk = ticketList.getHead();
                    if (!tk) {
                        outputLog += "(Chua co ve nào duoc dat trong he thong)\n";
                    }
                    while (tk != nullptr) {
                        outputLog += " [VE] Ma: " + tk->getTicketID() + 
                                     " | Tau: " + tk->getTrainCode() + 
                                     " | Toa: " + to_string(tk->getCarriageNo()) + 
                                     " | Ghe: " + to_string(tk->getSeatNo()) + 
                                     " | Chang: Ga " + to_string(tk->getFromStationID()) + " -> Ga " + to_string(tk->getToStationID()) + "\n";
                        tk = tk->getNextTicket();
                    }
                    break;
                }

                // CLICK NÚT 5: Khởi chạy Mô phỏng Đồ họa 2D
                case 5: {
                    outputLog = "--> DANG MO CUA SO MO PHONG SFML 2D RUNNING ENGINE...\n";
                    GraphicalSimulationEngine::runSFMLSimulation(&stationList, &trainList);
                    outputLog += "-> Da ket thuc phien mo phong va tro ve Dashboard!";
                    break;
                }

                // CLICK NÚT 6: Tai lai du lieu tu File Text
                case 6: {
                    FileManager::loadStations("stations.txt", stationList);
                    FileManager::loadTrains("trains.txt", trainList);
                    FileManager::loadTickets("tickets.txt", ticketList);
                    outputLog = "-> NẠP LAI THANH CONG DU LIEU TU CÁC TAP TIN TEXT (PERSISTENCE)!\n";
                    break;
                }

                // CLICK NÚT 0: Thoát
                case 0:
                    window.close();
                    break;
                }
            }
        }

        // Vẽ màn hình
        window.clear(sf::Color(24, 25, 28));

        // Tiêu đề chính Dashboard
        sf::Text headerText("PBL2: DOKU DIEU PHOI TAU HOA - BANTUAN GRAPHICAL DASHBOARD", font, 20);
        headerText.setFillColor(sf::Color(241, 196, 15));
        headerText.setPosition(startX, 35.0f);
        window.draw(headerText);

        // Vẽ các Nút bấm
        for (auto& btn : buttons) {
            btn.draw(window);
        }

        // Vẽ Khung hiển thị thông tin Dữ liệu
        window.draw(outputBox);

        sf::Text logText(outputLog, font, 14);
        logText.setFillColor(sf::Color::White);
        logText.setPosition(425.0f, 115.0f);
        window.draw(logText);

        window.display();
    }
}
