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

// Show Input Dialog Pop-up
bool GuiMenu::showInputDialog(const string& title, const string& promptLabel, string& outputValue, sf::Font& font) {
    sf::RenderWindow dialog(sf::VideoMode({480, 220}), title, sf::Style::Titlebar | sf::Style::Close);
    dialog.setFramerateLimit(60);

    string inputText = "";
    sf::Text promptText(font, promptLabel, 14);
    promptText.setFillColor(sf::Color::White);
    promptText.setPosition(sf::Vector2f(30.0f, 25.0f));

    sf::RectangleShape inputBox(sf::Vector2f(420.0f, 40.0f));
    inputBox.setPosition(sf::Vector2f(30.0f, 60.0f));
    inputBox.setFillColor(sf::Color(20, 20, 25));
    inputBox.setOutlineThickness(2.0f);
    inputBox.setOutlineColor(sf::Color(52, 152, 219));

    sf::Text typedText(font, "", 16);
    typedText.setFillColor(sf::Color(241, 196, 15));
    typedText.setPosition(sf::Vector2f(40.0f, 68.0f));

    GuiButton btnConfirm(135.0f, 130.0f, 210.0f, 40.0f, "XAC NHAN (ENTER)", font, 99);

    while (dialog.isOpen()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(dialog);
        bool mouseClicked = false;

        while (const auto event = dialog.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                dialog.close();
                return false;
            }
            if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseBtn->button == sf::Mouse::Button::Left) mouseClicked = true;
            }
            if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()) {
                uint32_t unicode = textEntered->unicode;
                if (unicode == 8) {
                    if (!inputText.empty()) inputText.pop_back();
                } else if (unicode == 13) {
                    outputValue = inputText;
                    dialog.close();
                    return true;
                } else if (unicode >= 32 && unicode <= 126) {
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

        typedText.setString(inputText + "|");
        dialog.clear(sf::Color(35, 37, 42));
        dialog.draw(promptText);
        dialog.draw(inputBox);
        dialog.draw(typedText);
        btnConfirm.draw(dialog);
        dialog.display();
    }
    return false;
}

// SeatMap UI
void GuiMenu::showSeatMapUI(StationList& stationList, TrainList& trainList, TicketList& ticketList, sf::Font& font) {
    string trainCode, strCarriage, strFrom, strTo;
    if (!showInputDialog("PBL2: CHON TAU TRACUU SEAT MAP", "Nhap Ma Tau (Vi du: SE1):", trainCode, font) ||
        !showInputDialog("PBL2: CHON TOA TRACUU SEAT MAP", "Nhap So Toa (Vi du: 1):", strCarriage, font) ||
        !showInputDialog("PBL2: PHAN DOAN CHANG", "Nhap ID Ga Di (Vi du: 101):", strFrom, font) ||
        !showInputDialog("PBL2: PHAN DOAN CHANG", "Nhap ID Ga Den (Vi du: 105):", strTo, font)) return;

    int carriageNo = stoi(strCarriage), fromSt = stoi(strFrom), toSt = stoi(strTo);
    TrainNode* train = trainList.findTrainByCode(trainCode);
    if (!train) return;

    int totalSeats = train->getCarriageSeats(carriageNo);
    if (totalSeats == 0) return;

    sf::RenderWindow mapWin(sf::VideoMode({800, 500}), "PBL2: SO DO GHE NGOI (SEAT MAP MATRIX)", sf::Style::Titlebar | sf::Style::Close);
    mapWin.setFramerateLimit(60);

    while (mapWin.isOpen()) {
        while (const auto event = mapWin.pollEvent()) {
            if (event->is<sf::Event::Closed>()) mapWin.close();
        }
        mapWin.clear(sf::Color(30, 30, 35));

        sf::Text header(font, "SO DO GHE TOA " + to_string(carriageNo) + " - TAU " + trainCode + " (CHANG " + to_string(fromSt) + " -> " + to_string(toSt) + ")", 16);
        header.setFillColor(sf::Color(241, 196, 15));
        header.setPosition(sf::Vector2f(30.0f, 20.0f));
        mapWin.draw(header);

        for (int i = 1; i <= totalSeats; ++i) {
            int row = (i - 1) % 4, col = (i - 1) / 4;
            float x = 50.0f + col * 60.0f, y = 80.0f + row * 60.0f + (row >= 2 ? 20.0f : 0.0f);

            sf::RectangleShape seatBox(sf::Vector2f(48.0f, 48.0f));
            seatBox.setPosition(sf::Vector2f(x, y));

            bool available = ticketList.isSeatAvailable(&stationList, trainCode, carriageNo, i, fromSt, toSt);
            seatBox.setFillColor(available ? sf::Color(46, 204, 113) : sf::Color(231, 76, 60));
            mapWin.draw(seatBox);

            sf::Text seatNum(font, to_string(i), 12);
            seatNum.setFillColor(sf::Color::White);
            seatNum.setPosition(sf::Vector2f(x + 15.0f, y + 15.0f));
            mapWin.draw(seatNum);
        }
        mapWin.display();
    }
}

// Apply Delay UI
void GuiMenu::applyDelayUI(StationList& stationList, TrainList& trainList, sf::Font& font) {
    string trainCode, strStID, strDelay;
    if (showInputDialog("DELAY PROPAGATION", "Nhap Ma Tau bi tre (Vi du: SE1):", trainCode, font) &&
        showInputDialog("DELAY PROPAGATION", "Nhap ID Ga bat dau bi tre (Vi du: 102):", strStID, font) &&
        showInputDialog("DELAY PROPAGATION", "Nhap So phut tre (Vi du: 30):", strDelay, font)) 
    {
        try {
            // Gọi hàm xử lý 2 bước: Dời giờ + Tái lập lịch tránh tàu
            if (trainList.applyDelayAndReschedule(trainCode, stoi(strStID), stoi(strDelay), &stationList)) {
                FileManager::saveTrains("trains.txt", trainList);
            }
        } catch (...) {}
    }
}

// MAIN DASHBOARD VOI GIAO DIEN NHOM KHOA HOC (ACCORDION MENU)
void GuiMenu::runGUIMenu(StationList& stationList, TrainList& trainList, TicketList& ticketList) {
    sf::RenderWindow window(sf::VideoMode({1280, 750}), "PBL2: HE THONG QUAN LY LICH TRINH & DIEU PHOI TAU HOA");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("arial.ttf")) {
        cout << "[Loi] Khong tim thay font 'arial.ttf'!\n";
    }

    float startX = 30.0f, startY = 80.0f, btnW = 350.0f, mainH = 42.0f, subH = 32.0f, gapY = 8.0f;
    vector<MenuGroup> menuGroups;

    // 1. Nhóm Ga
    MenuGroup g1(startX, 0, btnW, mainH, "[1] QUAN LY GA DONG SAT", font, 100);
    g1.subButtons.push_back(GuiButton(startX + 20.0f, 0, btnW - 20.0f, subH, "1.1. Hien thi danh sach cac Ga", font, 1));
    g1.subButtons.push_back(GuiButton(startX + 20.0f, 0, btnW - 20.0f, subH, "1.2. Them Ga moi tao tuyen", font, 2));
    g1.subButtons.push_back(GuiButton(startX + 20.0f, 0, btnW - 20.0f, subH, "1.3. Tim kiem Ga theo ID", font, 3));
    menuGroups.push_back(g1);

    // 2. Nhóm Tàu
    MenuGroup g2(startX, 0, btnW, mainH, "[2] QUAN LY TAU & LICH TRINH", font, 200);
    g2.subButtons.push_back(GuiButton(startX + 20.0f, 0, btnW - 20.0f, subH, "2.1. Hien thi danh sach Tau", font, 4));
    g2.subButtons.push_back(GuiButton(startX + 20.0f, 0, btnW - 20.0f, subH, "2.2. Tra cuu chi tiet Toa & Lịch", font, 5));
    g2.subButtons.push_back(GuiButton(startX + 20.0f, 0, btnW - 20.0f, subH, "2.3. Them chuyen Tau moi", font, 6));
    g2.subButtons.push_back(GuiButton(startX + 20.0f, 0, btnW - 20.0f, subH, "2.4. Cap nhat tre (Delay Propagate)", font, 11));
    menuGroups.push_back(g2);

    // 3. Nhóm Vé
    MenuGroup g3(startX, 0, btnW, mainH, "[3] QUAN LY VE & GHE NGOI", font, 300);
    g3.subButtons.push_back(GuiButton(startX + 20.0f, 0, btnW - 20.0f, subH, "3.1. Danh sach ve da phat hanh", font, 7));
    g3.subButtons.push_back(GuiButton(startX + 20.0f, 0, btnW - 20.0f, subH, "3.2. Dat ve (Segment Booking)", font, 8));
    g3.subButtons.push_back(GuiButton(startX + 20.0f, 0, btnW - 20.0f, subH, "3.3. Huy ve da phat hanh", font, 9));
    g3.subButtons.push_back(GuiButton(startX + 20.0f, 0, btnW - 20.0f, subH, "3.4. In so do ghe (Seat Map 2D)", font, 10));
    menuGroups.push_back(g3);

    // 4. Nhóm Mô phỏng & Tệp
    MenuGroup g4(startX, 0, btnW, mainH, "[4] MO PHONG & HE THONG", font, 400);
    g4.subButtons.push_back(GuiButton(startX + 20.0f, 0, btnW - 20.0f, subH, "4.1. Mo phong 2D Running Engine", font, 12));
    g4.subButtons.push_back(GuiButton(startX + 20.0f, 0, btnW - 20.0f, subH, "4.2. Reload du lieu tu File Text", font, 13));
    menuGroups.push_back(g4);

    // 5. Nút Thoát
    MenuGroup g5(startX, 0, btnW, mainH, "[0] THOAT CHUONG TRINH", font, 0);
    menuGroups.push_back(g5);

    sf::RectangleShape outputBox(sf::Vector2f(840.0f, 620.0f));
    outputBox.setPosition(sf::Vector2f(410.0f, 80.0f));
    outputBox.setFillColor(sf::Color(30, 30, 35));
    outputBox.setOutlineThickness(2.0f);
    outputBox.setOutlineColor(sf::Color(100, 100, 100));

    string outputLog = "WELCOME TO PBL2 DASHBOARD\n-----------------------------------\nHay chon mot muc menu ben trai de thao tac.";

    while (window.isOpen()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        bool mouseClicked = false;

        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseBtn->button == sf::Mouse::Button::Left) mouseClicked = true;
            }
        }

        // Tự động tính toán lại vị trí Y cho các nhóm menu khi thả xuống
        float currentY = startY;
        for (auto& group : menuGroups) {
            group.mainCategoryBtn.updatePosition(startX, currentY);
            currentY += mainH + gapY;

            if (group.isExpanded) {
                for (auto& subBtn : group.subButtons) {
                    subBtn.updatePosition(startX + 15.0f, currentY);
                    currentY += subH + 4.0f;
                }
                currentY += 4.0f;
            }
        }

        // Xử lý Sự kiện Click Nút
        for (auto& group : menuGroups) {
            group.mainCategoryBtn.updateHover(mousePos);

            if (group.mainCategoryBtn.isClicked(mousePos, mouseClicked)) {
                if (group.mainCategoryBtn.actionID == 0) {
                    window.close();
                } else {
                    group.isExpanded = !group.isExpanded;
                    group.mainCategoryBtn.setSelected(group.isExpanded);
                }
            }

            if (group.isExpanded) {
                for (auto& subBtn : group.subButtons) {
                    subBtn.updateHover(mousePos);

                    if (subBtn.isClicked(mousePos, mouseClicked)) {
                        switch (subBtn.actionID) {
                        case 1: {
                            outputLog = "=== DANH SACH CAC GA DONG SAT TREN TUYEN ===\n\n";
                            StationNode* st = stationList.getHead();
                            while (st) {
                                outputLog += " [GA] ID: " + to_string(st->getStationID()) + 
                                             " | Ten Ga: " + st->getName() + 
                                             " | Moc Km: " + to_string((int)st->getKmMarker()) + 
                                             " km | So Ray Phu: " + to_string(st->getNumberOfTracks()) + "\n";
                                st = st->getNextStation();
                            }
                            break;
                        }
                        case 2: {
                            string strID, name, strKm, strTracks;
                            if (showInputDialog("PBL2: THEM GA MOI", "1/4. Nhap ID Ga (VD: 109):", strID, font) &&
                                showInputDialog("PBL2: THEM GA MOI", "2/4. Nhap Ten Ga (VD: Hue):", name, font) &&
                                showInputDialog("PBL2: THEM GA MOI", "3/4. Nhap Moc Km (VD: 688):", strKm, font) &&
                                showInputDialog("PBL2: THEM GA MOI", "4/4. Nhap So Ray Phu (VD: 3):", strTracks, font)) 
                            {
                                try {
                                    stationList.addStation(stoi(strID), name, stod(strKm), stoi(strTracks));
                                    FileManager::saveStations("stations.txt", stationList);
                                    outputLog = "-> [THANH CONG] DA THEM GA " + name + " TRUC TIEP TREN GUI!";
                                } catch (...) { outputLog = "-> [LOI] DU LIEU NHAP VAO KHONG HOP LE!"; }
                            }
                            break;
                        }
                        case 3: {
                            string strID;
                            if (showInputDialog("PBL2: TIM KIEM GA", "Nhap ID Ga can tim:", strID, font)) {
                                try {
                                    StationNode* st = stationList.findStationByID(stoi(strID));
                                    if (st) {
                                        outputLog = "=== KET QUA TIM KIEM GA ===\n\n [GA TIM THAY] ID: " + to_string(st->getStationID()) + 
                                                     " | Ten: " + st->getName() + " | Km: " + to_string((int)st->getKmMarker()) + 
                                                     " | Ray Phu: " + to_string(st->getNumberOfTracks());
                                    } else outputLog = "-> KHONG TIM THAY GA CO ID TRÊN!";
                                } catch (...) { outputLog = "-> [LOI] ID PHAI LA SO NGUYEN!"; }
                            }
                            break;
                        }
                        case 4: {
                            outputLog = "=== DANH SACH CAC CHUYEN TAU CHAY TREN TUYEN ===\n\n";
                            TrainNode* tr = trainList.getHead();
                            while (tr) {
                                outputLog += " [TAU] Ma: " + tr->getTrainCode() + " | Huong: " + (tr->getDirection() == 1 ? "Nam (Di vao)" : "Bac (Di ra)") + "\n";
                                tr = tr->getNextTrain();
                            }
                            break;
                        }
                        case 5: {
                            outputLog = "========================================================================\n BANG CHI TIET LICH TRINH & TOA XE\n========================================================================\n\n";
                            TrainNode* tr = trainList.getHead();
                            while (tr) {
                                outputLog += " [TAU " + tr->getTrainCode() + "] | Huong: " + (tr->getDirection() == 1 ? "Nam" : "Bac") + "\n   * Toa: ";
                                CarriageNode* c = tr->getHeadCarriage();
                                while (c) {
                                    outputLog += "[Toa " + to_string(c->getCarriageNo()) + ": " + c->getType() + " (" + to_string(c->getTotalSeats()) + "g)]  ";
                                    c = c->getNextCarriage();
                                }
                                outputLog += "\n   * Lịch dừng:\n";
                                StopScheduleNode* s = tr->getHeadSchedule();
                                while (s) {
                                    char buf[100];
                                    snprintf(buf, sizeof(buf), "     + Ga %-3d | Den: %02d:%02d -> Di: %02d:%02d | Ray: %d\n",
                                             s->getStationID(), s->getArrivalMinute()/60, s->getArrivalMinute()%60, s->getDepartureMinute()/60, s->getDepartureMinute()%60, s->getTrackUsed());
                                    outputLog += string(buf);
                                    s = s->getNextStop();
                                }
                                outputLog += "------------------------------------------------------------------------\n";
                                tr = tr->getNextTrain();
                            }
                            break;
                        }
                        case 6: {
                            string code, strDir;
                            if (showInputDialog("PBL2: THEM TAU MOI", "1/2. Nhap Ma Tau (VD: SE5):", code, font) &&
                                showInputDialog("PBL2: THEM TAU MOI", "2/2. Huong Chay (1: Nam, 0: Bac):", strDir, font)) 
                            {
                                try {
                                    trainList.addTrain(code, stoi(strDir));
                                    FileManager::saveTrains("trains.txt", trainList);
                                    outputLog = "-> [THANH CONG] DA THEM TAU " + code;
                                } catch (...) { outputLog = "-> [LOI] HUONG CHAY PHAI LA 1 HOAC 0!"; }
                            }
                            break;
                        }
                        case 7: {
                            outputLog = "=== DANH SACH VE DA PHAT HANH TRONG HE THONG ===\n\n";
                            TicketNode* tk = ticketList.getHead();
                            if (!tk) outputLog += "(Chua co ve nào trong he thong)\n";
                            while (tk) {
                                outputLog += " [VE] Ma: " + tk->getTicketID() + " | Tau: " + tk->getTrainCode() + " | Toa: " + to_string(tk->getCarriageNo()) + 
                                             " | Ghe: " + to_string(tk->getSeatNo()) + " | Chang: Ga " + to_string(tk->getFromStationID()) + " -> Ga " + to_string(tk->getToStationID()) + "\n";
                                tk = tk->getNextTicket();
                            }
                            break;
                        }
                        case 8: {
                            string tID, trCode, strCNo, strSNo, strFrom, strTo;
                            if (showInputDialog("BOOKING", "1/6. Nhap Ma Ve (VD: TCK201):", tID, font) &&
                                showInputDialog("BOOKING", "2/6. Nhap Ma Tau (VD: SE1):", trCode, font) &&
                                showInputDialog("BOOKING", "3/6. Nhap So Toa (VD: 1):", strCNo, font) &&
                                showInputDialog("BOOKING", "4/6. Nhap So Ghe (VD: 5):", strSNo, font) &&
                                showInputDialog("BOOKING", "5/6. Nhap ID Ga Di (VD: 101):", strFrom, font) &&
                                showInputDialog("BOOKING", "6/6. Nhap ID Ga Den (VD: 103):", strTo, font)) 
                            {
                                try {
                                    int cNo = stoi(strCNo), sNo = stoi(strSNo), fromSt = stoi(strFrom), toSt = stoi(strTo);
                                    if (ticketList.isSeatAvailable(&stationList, trCode, cNo, sNo, fromSt, toSt)) {
                                        ticketList.addTicket(tID, trCode, cNo, sNo, fromSt, toSt);
                                        FileManager::saveTickets("tickets.txt", ticketList);
                                        outputLog = "-> [THANH CONG] DA DAT VE " + tID + " CHANG GA " + strFrom + " -> GA " + strTo;
                                    } else outputLog = "-> [THAT BAI] GHE DA DUOC DAT TRONG PHAN DOAN CHANG NAY!";
                                } catch (...) { outputLog = "-> [LOI] DU LIEU NHAP KHONG HOP LE!"; }
                            }
                            break;
                        }
                        case 9: {
                            string tID;
                            if (showInputDialog("PBL2: HUY VE", "Nhap Ma Ve can huy (VD: TCK101):", tID, font)) {
                                if (ticketList.cancelTicket(tID)) {
                                    FileManager::saveTickets("tickets.txt", ticketList);
                                    outputLog = "-> [THANH CONG] DA HUY VE " + tID;
                                } else outputLog = "-> [LOI] KHONG TIM THAY MA VE " + tID;
                            }
                            break;
                        }
                        case 10: {
                            showSeatMapUI(stationList, trainList, ticketList, font);
                            break;
                        }
                        case 11: {
                            applyDelayUI(stationList, trainList, font);
                            outputLog = "-> [THANH CONG] DA HOAN THANH DELAY PROPAGATION VA TAI LAP LICH TRANH TAU!";
                            break;
                        }
                        case 12: {
                            outputLog = "--> DANG MO CUA SO MO PHONG SFML 2D RUNNING ENGINE...\n";
                            GraphicalSimulationEngine::runSFMLSimulation(&stationList, &trainList);
                            outputLog += "-> Da ket thuc phien mo phong va tro ve Dashboard!";
                            break;
                        }
                        case 13: {
                            FileManager::loadStations("stations.txt", stationList);
                            FileManager::loadTrains("trains.txt", trainList);
                            FileManager::loadTickets("tickets.txt", ticketList);
                            outputLog = "-> NAP LAI THANH CONG DU LIEU TU FILE PERSISTENCE!";
                            break;
                        }
                        }
                    }
                }
            }
        }

        window.clear(sf::Color(24, 25, 28));

        sf::Text headerText(font, "PBL2: DIEU PHOI TAU HOA", 18);
        headerText.setFillColor(sf::Color(241, 196, 15));
        headerText.setPosition(sf::Vector2f(startX, 30.0f));
        window.draw(headerText);

        // Vẽ các nhóm menu và các nút con mở rộng
        for (auto& group : menuGroups) {
            group.mainCategoryBtn.draw(window);
            if (group.isExpanded) {
                for (auto& subBtn : group.subButtons) {
                    subBtn.draw(window);
                }
            }
        }

        window.draw(outputBox);
        sf::Text logText(font, outputLog, 13);
        logText.setFillColor(sf::Color::White);
        logText.setPosition(sf::Vector2f(425.0f, 95.0f));
        window.draw(logText);

        window.display();
    }
}
