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

// Hàm tạo Cửa sổ Pop-up Nhập liệu Đồ họa (GUI Dialog)
bool GuiMenu::showInputDialog(const string& title, const string& promptLabel, string& outputValue, sf::Font& font) {
    sf::RenderWindow dialog(sf::VideoMode({450, 220}), title, sf::Style::Titlebar | sf::Style::Close);
    dialog.setFramerateLimit(60);

    string inputText = "";
    
    sf::Text promptText(font, promptLabel, 14);
    promptText.setFillColor(sf::Color::White);
    promptText.setPosition(sf::Vector2f(30.0f, 25.0f));

    // Khung ô nhập văn bản
    sf::RectangleShape inputBox(sf::Vector2f(390.0f, 40.0f));
    inputBox.setPosition(sf::Vector2f(30.0f, 60.0f));
    inputBox.setFillColor(sf::Color(20, 20, 25));
    inputBox.setOutlineThickness(2.0f);
    inputBox.setOutlineColor(sf::Color(52, 152, 219));

    sf::Text typedText(font, "", 16);
    typedText.setFillColor(sf::Color(241, 196, 15));
    typedText.setPosition(sf::Vector2f(40.0f, 68.0f));

    GuiButton btnConfirm(120.0f, 130.0f, 210.0f, 40.0f, "XAC NHAN (ENTER)", font, 99);
    bool confirmed = false;

    while (dialog.isOpen()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(dialog);
        bool mouseClicked = false;

        while (const auto event = dialog.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                dialog.close();
                return false;
            }

            if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseBtn->button == sf::Mouse::Button::Left) {
                    mouseClicked = true;
                }
            }

            // Xử lý sự kiện gõ bàn phím trực tiếp vào ô nhập SFML
            if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()) {
                uint32_t unicode = textEntered->unicode;
                if (unicode == 8) { // Phím Backspace (Xóa ký tự)
                    if (!inputText.empty()) inputText.pop_back();
                } else if (unicode == 13) { // Phím Enter (Xác nhận)
                    outputValue = inputText;
                    dialog.close();
                    return true;
                } else if (unicode >= 32 && unicode <= 126) { // Ký tự in được
                    inputText += static_cast<char>(unicode);
                }
            }
        }

        btnConfirm.updateHover(mousePos);
        if (btnConfirm.isClicked(mousePos, mouseClicked)) {
            outputValue = inputText;
            dialog.close();
            return true;
        }

        typedText.setString(inputText + "|"); // Con trỏ nhấp nháy

        dialog.clear(sf::Color(35, 37, 42));
        dialog.draw(promptText);
        dialog.draw(inputBox);
        dialog.draw(typedText);
        btnConfirm.draw(dialog);
        dialog.display();
    }
    return false;
}

void GuiMenu::runGUIMenu(StationList& stationList, TrainList& trainList, TicketList& ticketList) {
    sf::RenderWindow window(sf::VideoMode({1280, 750}), 
                            "PBL2: HE THONG QUAN LY LICH TRINH & DIEU PHOI TAU HOA");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("arial.ttf")) {
        cout << "[Loi] Khong tim thay font 'arial.ttf'!\n";
    }

    vector<GuiButton> buttons;
    float startX = 30.0f;
    float startY = 80.0f;
    float btnW = 350.0f;
    float btnH = 38.0f;
    float gapY = 10.0f;

    buttons.push_back(GuiButton(startX, startY + 0 * (btnH + gapY), btnW, btnH, "1. HIEN THI DANH SACH TAT CA CAC GA", font, 1));
    buttons.push_back(GuiButton(startX, startY + 1 * (btnH + gapY), btnW, btnH, "2. THEM GA MOI VAO TUYEN", font, 2));
    buttons.push_back(GuiButton(startX, startY + 2 * (btnH + gapY), btnW, btnH, "3. TIM KIEM GA THEO ID", font, 3));
    buttons.push_back(GuiButton(startX, startY + 3 * (btnH + gapY), btnW, btnH, "4. HIEN THI DANH SACH CAC CHUYEN TAU", font, 4));
    buttons.push_back(GuiButton(startX, startY + 4 * (btnH + gapY), btnW, btnH, "5. TRA CUU CHI TIET TAU (TOA XE & LICH DUNG)", font, 5));
    buttons.push_back(GuiButton(startX, startY + 5 * (btnH + gapY), btnW, btnH, "6. THEM CHUYEN TAU MOI", font, 6));
    buttons.push_back(GuiButton(startX, startY + 6 * (btnH + gapY), btnW, btnH, "7. HIEN THI DANH SACH TAT CA VE DA DAT", font, 7));
    buttons.push_back(GuiButton(startX, startY + 7 * (btnH + gapY), btnW, btnH, "8. HUY VE PHAT HANH", font, 8));
    buttons.push_back(GuiButton(startX, startY + 8 * (btnH + gapY), btnW, btnH, "9. MO CUA SO MO PHONG RUNNING 2D (>>>)", font, 9));
    buttons.push_back(GuiButton(startX, startY + 9 * (btnH + gapY), btnW, btnH, "10. RELOAD DU LIEU TU FILE PERSISTENCE", font, 10));
    buttons.push_back(GuiButton(startX, startY + 10 * (btnH + gapY), btnW, btnH, "0. THOAT CHUONG TRINH", font, 0));

    sf::RectangleShape outputBox(sf::Vector2f(840.0f, 620.0f));
    outputBox.setPosition(sf::Vector2f(410.0f, 80.0f));
    outputBox.setFillColor(sf::Color(30, 30, 35));
    outputBox.setOutlineThickness(2.0f);
    outputBox.setOutlineColor(sf::Color(100, 100, 100));

    string outputLog = "WELCOME TO PBL2\n";
    outputLog += "----------------------\n";

    while (window.isOpen()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        bool mouseClicked = false;

        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseBtn->button == sf::Mouse::Button::Left) {
                    mouseClicked = true;
                }
            }
        }

        for (auto& btn : buttons) {
            btn.updateHover(mousePos);

            if (btn.isClicked(mousePos, mouseClicked)) {
                switch (btn.actionID) {
                
                // 1. Hiển thị ga
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

                // 2. THÊM GA MOI BANG CỬA SỔ GUI POPUP
                case 2: {
                    string strID, name, strKm, strTracks;
                    if (showInputDialog("PBL2 GUI: THEM GA MOI", "1/4. Nhap ID Ga (Vi du: 109):", strID, font) &&
                        showInputDialog("PBL2 GUI: THEM GA MOI", "2/4. Nhap Ten Ga (Vi du: Hue):", name, font) &&
                        showInputDialog("PBL2 GUI: THEM GA MOI", "3/4. Nhap Moc Km (Vi du: 688):", strKm, font) &&
                        showInputDialog("PBL2 GUI: THEM GA MOI", "4/4. Nhap So Ray Phu (Vi du: 3):", strTracks, font)) 
                    {
                        try {
                            int id = stoi(strID);
                            double km = stod(strKm);
                            int tracks = stoi(strTracks);
                            
                            stationList.addStation(id, name, km, tracks);
                            FileManager::saveStations("stations.txt", stationList);
                            outputLog = "-> [THANH CONG] DA THEM GA " + name + " (ID: " + to_string(id) + ") TRUC TIEP TREN GUI!";
                        } catch (...) {
                            outputLog = "-> [LOI] DU LIEU NHAP VAO KHONG HOP LE!";
                        }
                    }
                    break;
                }

                // 3. TÌM KIẾM GA BẰNG GUI POPUP
                case 3: {
                    string strID;
                    if (showInputDialog("PBL2 GUI: TIM KIEM GA", "Nhap ID Ga can tim:", strID, font)) {
                        try {
                            int id = stoi(strID);
                            StationNode* st = stationList.findStationByID(id);
                            if (st) {
                                outputLog = "=== KET QUA TIM KIEM GA ===\n\n";
                                outputLog += " [GA TIM THAY] ID: " + to_string(st->getStationID()) + 
                                             " | Ten: " + st->getName() + 
                                             " | Km: " + to_string((int)st->getKmMarker()) + 
                                             " | So Ray Phu: " + to_string(st->getNumberOfTracks());
                            } else {
                                outputLog = "-> KHONG TIM THAY GA CO ID: " + to_string(id);
                            }
                        } catch (...) {
                            outputLog = "-> [LOI] ID PHAI LA SO NGUYEN!";
                        }
                    }
                    break;
                }

                // 4. Danh sách tàu
                case 4: {
                    outputLog = "=== DANH SACH CAC CHUYEN TAU CHAY TREN TUYEN ===\n\n";
                    TrainNode* tr = trainList.getHead();
                    while (tr != nullptr) {
                        outputLog += " [TAU] Ma Tau: " + tr->getTrainCode() + 
                                     " | Huong Chay: " + (tr->getDirection() == 1 ? "Nam (Di vao)" : "Bac (Di ra)") + "\n";
                        tr = tr->getNextTrain();
                    }
                    break;
                }

                // 5. Tra cứu chi tiết
                case 5: {
                    outputLog = "========================================================================\n";
                    outputLog += "                   BANG CHI TIET LICH TRINH & TOA XE                    \n";
                    outputLog += "========================================================================\n\n";
                    
                    TrainNode* tr = trainList.getHead();
                    while (tr != nullptr) {
                        outputLog += " [TAU " + tr->getTrainCode() + "] | Huong: " + (tr->getDirection() == 1 ? "Nam (Di vao)" : "Bac (Di ra)") + "\n";
                        
                        outputLog += "   * Danh sach Toa:\n     ";
                        CarriageNode* c = tr->getHeadCarriage();
                        while (c != nullptr) {
                            outputLog += "[Toa " + to_string(c->getCarriageNo()) + ": " + c->getType() + " (" + to_string(c->getTotalSeats()) + " ghe)]  ";
                            c = c->getNextCarriage();
                        }
                        outputLog += "\n";

                        outputLog += "   * Lich dung do cac ga:\n";
                        StopScheduleNode* s = tr->getHeadSchedule();
                        while (s != nullptr) {
                            int arrH = s->getArrivalMinute() / 60;
                            int arrM = s->getArrivalMinute() % 60;
                            int depH = s->getDepartureMinute() / 60;
                            int depM = s->getDepartureMinute() % 60;

                            char buf[100];
                            snprintf(buf, sizeof(buf), "     + Ga %-3d | Den: %02d:%02d -> Di: %02d:%02d | Ray: %d\n",
                                     s->getStationID(), arrH, arrM, depH, depM, s->getTrackUsed());
                            
                            outputLog += string(buf);
                            s = s->getNextStop();
                        }
                        outputLog += "------------------------------------------------------------------------\n";
                        tr = tr->getNextTrain();
                    }
                    break;
                }
                // 6. THÊM TÀU MỚI BẰNG GUI POPUP
                case 6: {
                    string code, strDir;
                    if (showInputDialog("PBL2 GUI: THEM TAU MOI", "1/2. Nhap Ma Tau (Vi du: SE5):", code, font) &&
                        showInputDialog("PBL2 GUI: THEM TAU MOI", "2/2. Huong Chay (1: Nam, 0: Bac):", strDir, font)) 
                    {
                        try {
                            int dir = stoi(strDir);
                            trainList.addTrain(code, dir);
                            FileManager::saveTrains("trains.txt", trainList);
                            outputLog = "-> [THANH CONG] DA THEM TAU " + code + " TRUC TIEP TREN GUI!";
                        } catch (...) {
                            outputLog = "-> [LOI] HUONG CHAY PHAI LA 1 HOAC 0!";
                        }
                    }
                    break;
                }

                // 7. Danh sách vé
                case 7: {
                    outputLog = "=== DANH SACH VE DA PHAT HANH TRONG HE THONG ===\n\n";
                    TicketNode* tk = ticketList.getHead();
                    if (!tk) outputLog += "(Chua co ve nao duoc dat trong he thong)\n";
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

                // 8. HỦY VÉ BẰNG GUI POPUP
                case 8: {
                    string tID;
                    if (showInputDialog("PBL2 GUI: HUY VE", "Nhap Ma Ve can huy (Vi du: TCK101):", tID, font)) {
                        if (ticketList.cancelTicket(tID)) {
                            FileManager::saveTickets("tickets.txt", ticketList);
                            outputLog = "-> [THANH CONG] DA HUY VE " + tID + " TRUC TIEP TREN GUI!";
                        } else {
                            outputLog = "-> [LOI] KHONG TIM THAY MA VE " + tID;
                        }
                    }
                    break;
                }

                // 9. Mô phỏng 2D
                case 9: {
                    outputLog = "--> DANG MO CUA SO MO PHONG SFML 2D RUNNING ENGINE...\n";
                    GraphicalSimulationEngine::runSFMLSimulation(&stationList, &trainList);
                    outputLog += "-> Da ket thuc phien mo phong va tro ve Dashboard!";
                    break;
                }

                // 10. Reload dữ liệu
                case 10: {
                    FileManager::loadStations("stations.txt", stationList);
                    FileManager::loadTrains("trains.txt", trainList);
                    FileManager::loadTickets("tickets.txt", ticketList);
                    outputLog = "-> NA P LAI THANH CONG DU LIEU TU CAC TAP TIN TEXT PERSISTENCE!";
                    break;
                }

                case 0:
                    window.close();
                    break;
                }
            }
        }

        window.clear(sf::Color(24, 25, 28));

        sf::Text headerText(font, "PBL2: DIEU PHOI TAU HOA", 18);
        headerText.setFillColor(sf::Color(241, 196, 15));
        headerText.setPosition(sf::Vector2f(startX, 30.0f));
        window.draw(headerText);

        for (auto& btn : buttons) {
            btn.draw(window);
        }

        window.draw(outputBox);

        sf::Text logText(font, outputLog, 13);
        logText.setFillColor(sf::Color::White);
        logText.setPosition(sf::Vector2f(425.0f, 95.0f));
        window.draw(logText);

        window.display();
    }
}
