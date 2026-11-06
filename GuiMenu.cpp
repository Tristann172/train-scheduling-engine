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
#include <iomanip>
#include <algorithm>

using namespace std;

// ==================== 1. HỘP THOẠI NHẬP LIỆU HIỆN ĐẠI (MODAL INPUT DIALOG) ====================
bool GuiMenu::showInputDialog(const string& title, const string& promptLabel, string& outputValue, sf::Font& font) {
    sf::RenderWindow dialog(sf::VideoMode({520, 230}), title, sf::Style::Titlebar | sf::Style::Close);
    dialog.setFramerateLimit(60);

    string inputText = "";

    sf::RectangleShape topBanner(sf::Vector2f(520.0f, 38.0f));
    topBanner.setFillColor(GuiTheme::CardBg);
    sf::Text bannerTitle(font, "NHAP THONG TIN HE THONG", 13);
    bannerTitle.setFillColor(GuiTheme::AccentGold);
    bannerTitle.setPosition(sf::Vector2f(20.0f, 10.0f));

    sf::Text promptText(font, promptLabel, 14);
    promptText.setFillColor(GuiTheme::TextLight);
    promptText.setPosition(sf::Vector2f(25.0f, 52.0f));

    sf::RectangleShape inputBox(sf::Vector2f(470.0f, 42.0f));
    inputBox.setPosition(sf::Vector2f(25.0f, 82.0f));
    inputBox.setFillColor(GuiTheme::BgMain);
    inputBox.setOutlineThickness(2.0f);
    inputBox.setOutlineColor(GuiTheme::AccentCyan);

    sf::Text typedText(font, "", 16);
    typedText.setFillColor(GuiTheme::AccentGold);
    typedText.setPosition(sf::Vector2f(35.0f, 92.0f));

    GuiButton btnConfirm;
    btnConfirm.init(75.0f, 150.0f, 175.0f, 40.0f, "XAC NHAN (ENTER)", font, 99, false);
    GuiButton btnCancel;
    btnCancel.init(270.0f, 150.0f, 175.0f, 40.0f, "HUY BO (ESC)", font, 98, true);

    sf::Clock blinkClock;

    while (dialog.isOpen()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(dialog);
        bool mouseClicked = false;

        while (const auto event = dialog.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                dialog.close();
                return false;
            }
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    dialog.close();
                    return false;
                }
            }
            if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseBtn->button == sf::Mouse::Button::Left) mouseClicked = true;
            }
            if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()) {
                uint32_t unicode = textEntered->unicode;
                if (unicode == 8) { // Backspace
                    if (!inputText.empty()) inputText.pop_back();
                } else if (unicode == 13) { // Enter
                    if (!inputText.empty()) {
                        outputValue = inputText;
                        dialog.close();
                        return true;
                    }
                } else if (unicode >= 32 && unicode <= 126) {
                    inputText += static_cast<char>(unicode);
                }
            }
        }

        btnConfirm.updateHover(mousePos);
        btnCancel.updateHover(mousePos);

        if (btnConfirm.isClicked(mouseClicked) && !inputText.empty()) {
            outputValue = inputText;
            dialog.close();
            return true;
        }
        if (btnCancel.isClicked(mouseClicked)) {
            dialog.close();
            return false;
        }

        bool showCursor = (static_cast<int>(blinkClock.getElapsedTime().asSeconds() * 2.0f) % 2 == 0);
        typedText.setString(inputText + (showCursor ? "_" : ""));

        dialog.clear(GuiTheme::PanelBg);
        dialog.draw(topBanner);
        dialog.draw(bannerTitle);
        dialog.draw(promptText);
        dialog.draw(inputBox);
        dialog.draw(typedText);
        btnConfirm.draw(dialog);
        btnCancel.draw(dialog);
        dialog.display();
    }
    return false;
}

// ==================== 2. SƠ ĐỒ GHẾ 2D TƯƠNG TÁC (CLICK ĐỂ ĐẶT VÉ TRỰC TIẾP) ====================
void GuiMenu::showSeatMapUI(StationList& stationList, TrainList& trainList, TicketList& ticketList, sf::Font& font, string& outputLog) {
    string trainCode, strCarriage, strFrom, strTo;
    if (!showInputDialog("SEAT MAP 2D", "1/4. Nhap Ma Tau (Vi du: SE1):", trainCode, font) ||
        !showInputDialog("SEAT MAP 2D", "2/4. Nhap So Toa (Vi du: 1):", strCarriage, font) ||
        !showInputDialog("SEAT MAP 2D", "3/4. Nhap ID Ga Di (Vi du: 101):", strFrom, font) ||
        !showInputDialog("SEAT MAP 2D", "4/4. Nhap ID Ga Den (Vi du: 105):", strTo, font)) return;

    int carriageNo = 0, fromSt = 0, toSt = 0;
    try {
        carriageNo = stoi(strCarriage);
        fromSt = stoi(strFrom);
        toSt = stoi(strTo);
    } catch (...) {
        outputLog = "-> [LOI NHAP LIEU] So toa va ID ga phai la so nguyen hop le!";
        return;
    }

    TrainNode* train = trainList.findTrainByCode(trainCode);
    if (!train) {
        outputLog = "-> [LOI TRA CUU] Khong tim thay doan tau co ma: " + trainCode;
        return;
    }

    int totalSeats = train->getCarriageSeats(carriageNo);
    if (totalSeats <= 0 || totalSeats > 128) {
        outputLog = "-> [LOI TOA XE] Toa so " + to_string(carriageNo) + " khong ton tai tren tau " + trainCode + "!";
        return;
    }

    // Kiểm tra sơ đồ ghế bằng truy vấn cây phân đoạn theo từng ghế.
    bool seatFree[129];
    if (!ticketList.checkCarriageSeats(&stationList, &trainList, trainCode, carriageNo, fromSt, toSt, seatFree, totalSeats)) {
        outputLog = "-> [LOI HANH TRINH] Tau " + trainCode + " KHONG phuc vu chang Ga " + to_string(fromSt) +
                    " -> Ga " + to_string(toSt) + "\n   (Nguyen nhan: Nhap nguoc chieu chay cua tau hoac tau khong dung tai ga nay).";
        return;
    }

    StationNode* stFromNode = stationList.findStationByID(fromSt);
    StationNode* stToNode   = stationList.findStationByID(toSt);
    string routeDesc = (stFromNode ? stFromNode->getName() : to_string(fromSt)) + " -> " +
                       (stToNode ? stToNode->getName() : to_string(toSt));

    sf::RenderWindow mapWin(sf::VideoMode({880, 520}), "PBL2: SO DO GHE NGOI TUONG TAC (INTERACTIVE SEAT MAP 2D)", sf::Style::Titlebar | sf::Style::Close);
    mapWin.setFramerateLimit(60);

    string statusBanner = "MEO: Click chuot trai truc tiep vao o GHE XANH de dat ve nhanh cho ghe do!";

    while (mapWin.isOpen()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(mapWin);
        bool mouseClicked = false;

        while (const auto event = mapWin.pollEvent()) {
            if (event->is<sf::Event::Closed>()) mapWin.close();
            if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseBtn->button == sf::Mouse::Button::Left) mouseClicked = true;
            }
        }

        mapWin.clear(GuiTheme::BgMain);

        // Header thông tin toa tàu
        sf::Text header(font, "SO DO GHE TOA " + to_string(carriageNo) + " | TAU " + trainCode +
                        " (P" + to_string(train->getPriority()) + ") | CHANG: " + routeDesc, 16);
        header.setFillColor(GuiTheme::AccentGold);
        header.setPosition(sf::Vector2f(35.0f, 20.0f));
        mapWin.draw(header);

        // Chú giải (Legend)
        int freeCount = 0;
        for (int i = 1; i <= totalSeats; ++i) if (seatFree[i]) freeCount++;

        sf::Text subHeader(font, "Tong so ghe: " + to_string(totalSeats) +
                           "  |  Con trong (Xanh): " + to_string(freeCount) +
                           "  |  Da dat (Do): " + to_string(totalSeats - freeCount), 13);
        subHeader.setFillColor(GuiTheme::AccentCyan);
        subHeader.setPosition(sf::Vector2f(35.0f, 48.0f));
        mapWin.draw(subHeader);

        // Khung thân toa tàu (Coach Hull)
        sf::RectangleShape carriageHull(sf::Vector2f(810.0f, 330.0f));
        carriageHull.setPosition(sf::Vector2f(35.0f, 85.0f));
        carriageHull.setFillColor(GuiTheme::PanelBg);
        carriageHull.setOutlineThickness(2.0f);
        carriageHull.setOutlineColor(GuiTheme::Border);
        mapWin.draw(carriageHull);

        // Lối đi giữa toa (Center Aisle)
        sf::RectangleShape aisle(sf::Vector2f(790.0f, 26.0f));
        aisle.setPosition(sf::Vector2f(45.0f, 237.0f));
        aisle.setFillColor(GuiTheme::BgMain);
        mapWin.draw(aisle);

        sf::Text aisleText(font, "--- LOI DI GIUA TOA TAU (AISLE) ---", 11);
        aisleText.setFillColor(GuiTheme::TextMuted);
        aisleText.setPosition(sf::Vector2f(330.0f, 243.0f));
        mapWin.draw(aisleText);

        // Vẽ ma trận ghế và bắt sự kiện click đặt vé trực tiếp
        int hoveredSeat = -1;
        for (int i = 1; i <= totalSeats; ++i) {
            int row = (i - 1) % 4;
            int col = (i - 1) / 4;
            float x = 55.0f + col * 48.0f;
            float y = 105.0f + row * 62.0f + (row >= 2 ? 32.0f : 0.0f);

            sf::RectangleShape seatBox(sf::Vector2f(40.0f, 46.0f));
            seatBox.setPosition(sf::Vector2f(x, y));

            bool available = seatFree[i];
            bool isHover = seatBox.getGlobalBounds().contains(sf::Vector2f(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)));
            if (isHover) hoveredSeat = i;

            seatBox.setFillColor(available ? sf::Color(39, 174, 96) : sf::Color(192, 57, 43));
            seatBox.setOutlineThickness(isHover ? 2.5f : 1.0f);
            seatBox.setOutlineColor(isHover ? GuiTheme::AccentGold : sf::Color(20, 20, 25));
            mapWin.draw(seatBox);

            // Gối tựa đầu ghế
            sf::RectangleShape headrest(sf::Vector2f(8.0f, 34.0f));
            headrest.setPosition(sf::Vector2f(x + 30.0f, y + 6.0f));
            headrest.setFillColor(available ? sf::Color(46, 204, 113) : sf::Color(231, 76, 60));
            mapWin.draw(headrest);

            sf::Text seatNum(font, to_string(i), 12);
            seatNum.setFillColor(sf::Color::White);
            seatNum.setPosition(sf::Vector2f(x + 8.0f, y + 14.0f));
            mapWin.draw(seatNum);

            // Nếu người dùng click vào ghế trống -> Mở hộp thoại nhập Mã Vé để đặt luôn!
            if (isHover && mouseClicked && available) {
                string newTicketID;
                if (showInputDialog("DAT VE NHANH TREN SO DO GHE",
                                    "Nhap Ma Ve moi cho Ghe so " + to_string(i) + " (VD: TCK305):",
                                    newTicketID, font)) 
                {
                    if (ticketList.findTicketByID(newTicketID) != nullptr) {
                        statusBanner = "[LOI] Ma ve " + newTicketID + " da ton tai trong he thong!";
                    } else if (ticketList.isSeatAvailable(&stationList, &trainList, trainCode, carriageNo, i, fromSt, toSt)) {
                        ticketList.addTicket(newTicketID, trainCode, carriageNo, i, fromSt, toSt);
                        FileManager::saveTickets("tickets.txt", ticketList);
                        seatFree[i] = false; // Cập nhật ngay trên giao diện
                        statusBanner = "[THANH CONG] Da phat hanh ve " + newTicketID + " cho Ghe " + to_string(i) + "!";
                        outputLog = "-> [SEAT MAP BOOKING] Da dat ve " + newTicketID + " (Tau " + trainCode +
                                    ", Toa " + to_string(carriageNo) + ", Ghe " + to_string(i) + ")";
                    }
                }
            }
        }

        // Thanh trạng thái dưới đáy cửa sổ Seat Map
        sf::RectangleShape footerBox(sf::Vector2f(810.0f, 45.0f));
        footerBox.setPosition(sf::Vector2f(35.0f, 435.0f));
        footerBox.setFillColor(GuiTheme::CardBg);
        footerBox.setOutlineThickness(1.0f);
        footerBox.setOutlineColor(GuiTheme::Border);
        mapWin.draw(footerBox);

        string hoverInfo = (hoveredSeat != -1)
            ? ("Dang chon Ghe so " + to_string(hoveredSeat) + ": " + (seatFree[hoveredSeat] ? "CON TRONG (Click de dat)" : "DA CO KHACH DAT"))
            : statusBanner;
        sf::Text footerTxt(font, hoverInfo, 13);
        footerTxt.setFillColor(GuiTheme::AccentGold);
        footerTxt.setPosition(sf::Vector2f(50.0f, 448.0f));
        mapWin.draw(footerTxt);

        mapWin.display();
    }
}

// ==================== 3. GIAO DIỆN LAN TRUYỀN TRỄ & TÁI LẬP LỊCH ====================
void GuiMenu::applyDelayUI(StationList& stationList, TrainList& trainList, sf::Font& font, string& outputLog) {
    string trainCode, strStID, strDelay;
    if (showInputDialog("DELAY PROPAGATION", "1/3. Nhap Ma Tau bi su co tre (Vi du: SE1):", trainCode, font) &&
        showInputDialog("DELAY PROPAGATION", "2/3. Nhap ID Ga bat dau bi tre (Vi du: 102):", strStID, font) &&
        showInputDialog("DELAY PROPAGATION", "3/3. Nhap So phut tre (Vi du: 30):", strDelay, font)) 
    {
        try {
            int stID = stoi(strStID);
            int delayMins = stoi(strDelay);
            if (trainList.applyDelayAndReschedule(trainCode, stID, delayMins, &stationList)) {
                FileManager::saveTrains("trains.txt", trainList);
                outputLog = "=== KET QUA LAN TRUYEN TRE & TAI LAP LICH (EVENT-DRIVEN) ===\n\n";
                outputLog += " -> Da cong them " + to_string(delayMins) + " phut cho tau " + trainCode + " tu Ga ID " + to_string(stID) + ".\n";
                outputLog += " -> Da kich hoat bo lap lich theo su kien (xet Priority & maxTracks).\n\n";
                outputLog += "Cac xung dot vua duoc giai quyet:\n";
                for (int i = 0; i < trainList.getLogCount(); ++i) {
                    outputLog += "   + " + trainList.getLog(i) + "\n";
                }
            } else {
                outputLog = "-> [THAT BAI] Khong tim thay Tau " + trainCode + " hoac Ga " + strStID + " khong nam trong lich trinh!";
            }
        } catch (...) {
            outputLog = "-> [LOI] ID Ga va So phut tre phai la so nguyen!";
        }
    }
}

// ==================== 4. MAIN DASHBOARD CONTROL CENTER ====================
void GuiMenu::runGUIMenu(StationList& stationList, TrainList& trainList, TicketList& ticketList) {
    sf::RenderWindow window(sf::VideoMode({1360, 768}), "PBL2: TRUNG TAM DIEU HANH & QUAN LY DUONG SAT DON (CONTROL CENTER)");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("arial.ttf")) {
        cout << "[Loi] Khong tim thay font 'arial.ttf'!\n";
    }

    // Tự động chạy giải tỏa xung đột ngay khi mở Dashboard
    trainList.resolveConflictAndScheduleSiding(&stationList);

    const float startX = 25.0f, startY = 105.0f, btnW = 340.0f, mainH = 40.0f, subH = 31.0f, gapY = 7.0f;
    static const int NUM_GROUPS = 5;
    MenuGroup menuGroups[NUM_GROUPS];

    // 1. Nhóm Quản lý Hạ tầng Ga (maxTracks)
    menuGroups[0].initGroup(startX, 0, btnW, mainH, "[1] QUAN LY HA TANG GA (MAX TRACKS)", font, 100);
    menuGroups[0].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "1.1. Danh sach Ga & Nang luc Ray (maxTracks)", font, 1);
    menuGroups[0].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "1.2. Them Ga moi vao tuyen", font, 2);
    menuGroups[0].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "1.3. Tra cuu chi tiet Ga theo ID", font, 3);
    menuGroups[0].isExpanded = true; // Mặc định mở nhóm 1
    menuGroups[0].mainCategoryBtn.setSelected(true);

    // 2. Nhóm Quản lý Đoàn tàu, Toa xe & Lịch trình (Bao quát 100% hàm trong TrainList)
    menuGroups[1].initGroup(startX, 0, btnW, mainH, "[2] QUAN LY TAU, TOA XE & LICH TRINH", font, 200);
    menuGroups[1].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "2.1. Danh sach Doan tau & Cap uu tien (P1-P3)", font, 4);
    menuGroups[1].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "2.2. Bang chi tiet Toa xe & Lich dung", font, 5);
    menuGroups[1].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "2.3. Them Doan tau moi (kem Priority)", font, 6);
    menuGroups[1].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "2.4. Gan Toa xe moi vao Doan tau", font, 14);
    menuGroups[1].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "2.5. Them Diem dung (Stop Schedule) cho Tau", font, 15);
    menuGroups[1].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "2.6. Bao cao Tre gio & Tai lap lich (Delay)", font, 11);

    // 3. Nhóm Quản lý Vé & Sơ đồ ghế 2D
    menuGroups[2].initGroup(startX, 0, btnW, mainH, "[3] QUAN LY VE & SEAT MAP 2D", font, 300);
    menuGroups[2].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "3.1. Danh sach Ve da phat hanh", font, 7);
    menuGroups[2].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "3.2. Dat ve theo phan doan (Segment Booking)", font, 8);
    menuGroups[2].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "3.3. Tra cuu Ve theo Ma Ve (Find by ID)", font, 16);
    menuGroups[2].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "3.4. Huy ve da phat hanh (Cancel Ticket)", font, 9);
    menuGroups[2].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "3.5. Mo So do ghe 2D (Click de dat ve)", font, 10);

    // 4. Nhóm Điều phối Xung đột & Mô phỏng Đồ họa
    menuGroups[3].initGroup(startX, 0, btnW, mainH, "[4] DIEU PHOI XUNG DOT & MO PHONG 2D", font, 400);
    menuGroups[3].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "4.1. Chay Mo phong Do hoa SFML 2D Engine", font, 12);
    menuGroups[3].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "4.2. Quet Xung dot & Phan luong Ray phu ngay", font, 17);
    menuGroups[3].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "4.3. Nap lai du lieu goc tu File Text", font, 13);

    // 5. Thoát
    menuGroups[4].initGroup(startX, 0, btnW, mainH, "[0] LUU DU LIEU & THOAT HE THONG", font, 0);

    string outputLog =
        "HE THONG DIEU PHOI DUONG SAT DON (MEET & PASS CONTROL CENTER)\n"
        "====================================================================\n"
        " * Thuat toan Uu tien Tau (Train Priority): SE (P1) > TN (P2) > Tau hang (P3).\n"
        " * Kiem soat Nang luc Ha tang (maxTracks): Tu dong giu tau o ga truoc neu ga xep chi co 1 ray.\n"
        " * Giai toa xung dot theo dong su kien thoi gian, tu dong phan luong ray.\n\n"
        "-> Hay chon mot chuc nang o bang dieu khien ben trai. Dung con lan chuot de cuon noi dung dai.";

    float scrollOffsetY = 0.0f;

    while (window.isOpen()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        bool mouseClicked = false;

        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseBtn->button == sf::Mouse::Button::Left) mouseClicked = true;
            }
            // Hỗ trợ lăn chuột để cuộn khung nội dung bên phải
            if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
                scrollOffsetY += wheel->delta * 28.0f;
                if (scrollOffsetY > 0.0f) scrollOffsetY = 0.0f;
            }
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Down) scrollOffsetY -= 28.0f;
                if (keyPressed->code == sf::Keyboard::Key::Up) {
                    scrollOffsetY += 28.0f;
                    if (scrollOffsetY > 0.0f) scrollOffsetY = 0.0f;
                }
            }
        }

        // Cập nhật tọa độ Y cho Accordion Menu
        float currentY = startY;
        for (int g = 0; g < NUM_GROUPS; ++g) {
            menuGroups[g].mainCategoryBtn.updatePosition(startX, currentY);
            currentY += mainH + gapY;

            if (menuGroups[g].isExpanded) {
                for (int s = 0; s < menuGroups[g].subCount; ++s) {
                    menuGroups[g].subButtons[s].updatePosition(startX + 15.0f, currentY);
                    currentY += subH + 4.0f;
                }
                currentY += 5.0f;
            }
        }

        // Xử lý sự kiện Click Menu
        for (int g = 0; g < NUM_GROUPS; ++g) {
            menuGroups[g].mainCategoryBtn.updateHover(mousePos);

            if (menuGroups[g].mainCategoryBtn.isClicked(mouseClicked)) {
                if (menuGroups[g].mainCategoryBtn.actionID == 0) {
                    window.close();
                } else {
                    bool willExpand = !menuGroups[g].isExpanded;
                    // Thu gọn các nhóm khác để giao diện gọn gàng, không tràn chiều dọc
                    for (int k = 0; k < NUM_GROUPS; ++k) {
                        menuGroups[k].isExpanded = false;
                        menuGroups[k].mainCategoryBtn.setSelected(false);
                    }
                    menuGroups[g].isExpanded = willExpand;
                    menuGroups[g].mainCategoryBtn.setSelected(willExpand);
                }
            }

            if (menuGroups[g].isExpanded) {
                for (int s = 0; s < menuGroups[g].subCount; ++s) {
                    GuiButton& subBtn = menuGroups[g].subButtons[s];
                    subBtn.updateHover(mousePos);

                    if (subBtn.isClicked(mouseClicked)) {
                        scrollOffsetY = 0.0f; // Reset vị trí cuộn khi chuyển mục

                        switch (subBtn.actionID) {
                        case 1: {
                            outputLog = "=== DANH SACH NHA GA & NANG LUC HA TANG (MAX TRACKS) ===\n\n";
                            for (LinkedList<StationNode*>::Node* station =
                                     stationList.getHead();
                                 station; station = station->next) {
                                StationNode* st = station->data;
                                string infraType = (st->getMaxTracks() == 1) ? " [Ga xep 1 ray - Khong the tranh tau]" : " [Ga co ray phu tranh tau]";
                                outputLog += " * Ga ID: " + to_string(st->getStationID()) +
                                             " | Ten: " + st->getName() +
                                             " | Ly trinh: " + to_string(static_cast<int>(st->getKmMarker())) + " km" +
                                             " | So ray (maxTracks): " + to_string(st->getMaxTracks()) + infraType + "\n";
                            }
                            break;
                        }
                        case 2: {
                            activeViewMode = 0;
                            string strID, name, strKm, strTracks;
                            if (showInputDialog("THEM GA MOI", "1/4. Nhap ID Ga (VD: 109):", strID, font) &&
                                showInputDialog("THEM GA MOI", "2/4. Nhap Ten Ga (VD: Hue):", name, font) &&
                                showInputDialog("THEM GA MOI", "3/4. Nhap Moc Km (VD: 688):", strKm, font) &&
                                showInputDialog("THEM GA MOI", "4/4. Nhap Tong so Ray maxTracks (1 = Ga xep, >=2 = Co ray phu):", strTracks, font)) 
                            {
                                try {
                                    stationList.addStation(stoi(strID), name, stod(strKm), stoi(strTracks));
                                    FileManager::saveStations("stations.txt", stationList);
                                    outputLog = "-> [THANH CONG] Da them Ga " + name + " (maxTracks = " + strTracks + ") va luu file!";
                                } catch (...) { outputLog = "-> [LOI] Du lieu nhap vao khong hop le!"; }
                            }
                            break;
                        }
                        case 3: {
                            activeViewMode = 0;
                            string strID;
                            if (showInputDialog("TIM KIEM GA", "Nhap ID Ga can tra cuu:", strID, font)) {
                                try {
                                    StationNode* st = stationList.findStationByID(stoi(strID));
                                    if (st) {
                                        outputLog = "=== THONG TIN CHI TIET NHA GA ===\n\n"
                                                    " * ID Ga          : " + to_string(st->getStationID()) + "\n"
                                                    " * Ten Nha Ga     : " + st->getName() + "\n"
                                                    " * Moc Km         : " + to_string(static_cast<int>(st->getKmMarker())) + " km\n"
                                                    " * Nang luc Ray   : " + to_string(st->getMaxTracks()) + " ray (" +
                                                    (st->getMaxTracks() > 1 ? "Co " + to_string(st->getMaxTracks() - 1) + " ray phu)" : "Chi co 1 ray chinh)");
                                    } else outputLog = "-> [KHONG TIM THAY] Khong co ga nao mang ID: " + strID;
                                } catch (...) { outputLog = "-> [LOI] ID Ga phai la so nguyen!"; }
                            }
                            break;
                        }
                        case 4: {
                            outputLog = "=== DANH SACH DOAN TAU & CAP DO UU TIEN (TRAIN PRIORITY) ===\n\n";
                            for (LinkedList<TrainNode*>::Node* train =
                                     trainList.getHead();
                                 train; train = train->next) {
                                TrainNode* tr = train->data;
                                string prioDesc = (tr->getPriority() == 1) ? "Uu tien 1 (Tau khach nhanh SE)" :
                                                  (tr->getPriority() == 2 ? "Uu tien 2 (Tau khach thuong TN)" : "Uu tien 3 (Tau hang)");
                                outputLog += " * Ma Tau: " + tr->getTrainCode() +
                                             "  |  Cap do: " + prioDesc +
                                             "  |  Huong chay: " + (tr->getDirection() == 1 ? "Bac -> Nam (1)" : "Nam -> Bac (0)") + "\n";
                            }
                            break;
                        }
                        case 5: {
                            outputLog = "=== BANG CHI TIET TOA XE & LICH TRINH DUNG DO (CUON CHUOT DE XEM TIEP) ===\n\n";
                            for (LinkedList<TrainNode*>::Node* train =
                                     trainList.getHead();
                                 train; train = train->next) {
                                TrainNode* tr = train->data;
                                outputLog += " [TAU " + tr->getTrainCode() + " - PRIORITY P" + to_string(tr->getPriority()) +
                                             " | Huong: " + (tr->getDirection() == 1 ? "Nam" : "Bac") + "]\n   * Danh sach Toa: ";
                                LinkedList<CarriageNode*>::Node* carriage =
                                    tr->getHeadCarriage();
                                if (!carriage) outputLog += "(Chua co toa xe)";
                                while (carriage) {
                                    CarriageNode* c = carriage->data;
                                    outputLog += "[Toa " + to_string(c->getCarriageNo()) + ": " + c->getType() + " (" + to_string(c->getTotalSeats()) + " ghe)]  ";
                                    carriage = carriage->next;
                                }
                                outputLog += "\n   * Lich trinh cac ga:\n";
                                LinkedList<StopScheduleNode*>::Node* stop =
                                    tr->getHeadSchedule();
                                while (stop) {
                                    StopScheduleNode* s = stop->data;
                                    StationNode* stNode = stationList.findStationByID(s->getStationID());
                                    string stName = stNode ? stNode->getName() : ("Ga " + to_string(s->getStationID()));
                                    char buf[140];
                                    snprintf(buf, sizeof(buf), "     + ID %-3d (%-10s) | Den: %02d:%02d -> Di: %02d:%02d | Dung Ray: %d %s\n",
                                             s->getStationID(), stName.c_str(),
                                             s->getArrivalMinute() / 60, s->getArrivalMinute() % 60,
                                             s->getDepartureMinute() / 60, s->getDepartureMinute() % 60,
                                             s->getTrackUsed(),
                                             (s->getTrackUsed() > 1 ? "(Ray phu tranh tau)" : "(Ray chinh)"));
                                    outputLog += string(buf);
                                    stop = stop->next;
                                }
                                outputLog += "--------------------------------------------------------------------------------\n";
                            }
                            break;
                        }
                        case 6: {
                            string code, strDir, strPrio;
                            if (showInputDialog("THEM TAU MOI", "1/3. Nhap Ma Tau (VD: SE5, TN3, HH1):", code, font) &&
                                showInputDialog("THEM TAU MOI", "2/3. Huong Chay (1: Bac->Nam, 0: Nam->Bac):", strDir, font) &&
                                showInputDialog("THEM TAU MOI", "3/3. Cap Uu Tien (1: SE, 2: TN, 3: Tau Hang, 0: Tu suy luan):", strPrio, font)) 
                            {
                                try {
                                    int prio = stoi(strPrio);
                                    trainList.addTrain(code, stoi(strDir), prio <= 0 ? -1 : prio);
                                    FileManager::saveTrains("trains.txt", trainList);
                                    TrainNode* added = trainList.findTrainByCode(code);
                                    outputLog = "-> [THANH CONG] Da them Tau " + code + " voi Cap uu tien P" +
                                                to_string(added ? added->getPriority() : 1) + "!";
                                } catch (...) { outputLog = "-> [LOI] Du lieu nhap vao khong hop le!"; }
                            }
                            break;
                        }
                        case 14: { // Bổ sung tính năng addCarriageToTrain có sẵn trong TrainList
                            string code, strCNo, type, strSeats;
                            if (showInputDialog("GAN TOA XE VAO TAU", "1/4. Nhap Ma Tau (VD: SE1):", code, font) &&
                                showInputDialog("GAN TOA XE VAO TAU", "2/4. Nhap So thu tu Toa (VD: 3):", strCNo, font) &&
                                showInputDialog("GAN TOA XE VAO TAU", "3/4. Loai Toa (VD: NgoiMem, GiuongNam):", type, font) &&
                                showInputDialog("GAN TOA XE VAO TAU", "4/4. Tong so ghe (VD: 48):", strSeats, font)) 
                            {
                                try {
                                    if (!trainList.findTrainByCode(code)) {
                                        outputLog = "-> [LOI] Khong tim thay doan tau " + code + "!";
                                    } else {
                                        trainList.addCarriageToTrain(code, stoi(strCNo), type, stoi(strSeats));
                                        FileManager::saveTrains("trains.txt", trainList);
                                        outputLog = "-> [THANH CONG] Da gan Toa so " + strCNo + " (" + type + ", " + strSeats + " ghe) vao tau " + code + "!";
                                    }
                                } catch (...) { outputLog = "-> [LOI] So toa va so ghe phai la so nguyen!"; }
                            }
                            break;
                        }
                        case 15: { // Bổ sung tính năng addStopScheduleToTrain có sẵn trong TrainList
                            string code, strStID, strArr, strDep;
                            if (showInputDialog("THEM LICH DUNG GA", "1/4. Nhap Ma Tau (VD: SE1):", code, font) &&
                                showInputDialog("THEM LICH DUNG GA", "2/4. Nhap ID Ga dung (VD: 104):", strStID, font) &&
                                showInputDialog("THEM LICH DUNG GA", "3/4. Phut den ga trong ngay (VD: 360 cho 06:00):", strArr, font) &&
                                showInputDialog("THEM LICH DUNG GA", "4/4. Phut roi ga trong ngay (VD: 375 cho 06:15):", strDep, font)) 
                            {
                                try {
                                    if (!trainList.findTrainByCode(code) || !stationList.findStationByID(stoi(strStID))) {
                                        outputLog = "-> [LOI] Ma tau hoac ID Ga khong ton tai!";
                                    } else {
                                        trainList.addStopScheduleToTrain(code, stoi(strStID), stoi(strArr), stoi(strDep), 1);
                                        trainList.resolveConflictAndScheduleSiding(&stationList);
                                        FileManager::saveTrains("trains.txt", trainList);
                                        outputLog = "-> [THANH CONG] Da them lich dung tai Ga " + strStID + " cho tau " + code + " va tu dong kiem tra xung dot!";
                                    }
                                } catch (...) { outputLog = "-> [LOI] Du lieu thoi gian phai la so nguyen!"; }
                            }
                            break;
                        }
                        case 7: {
                            outputLog = "=== DANH SACH VE DA PHAT HANH TRONG HE THONG ===\n\n";
                            LinkedList<TicketNode*>::Node* ticket =
                                ticketList.getHead();
                            if (!ticket) outputLog += " (Chua co ve nao duoc dat trong he thong)\n";
                            while (ticket) {
                                TicketNode* tk = ticket->data;
                                outputLog += " * Ma Ve: " + tk->getTicketID() +
                                             " | Tau: " + tk->getTrainCode() +
                                             " | Toa: " + to_string(tk->getCarriageNo()) +
                                             " | Ghe: " + to_string(tk->getSeatNo()) +
                                             " | Hanh trinh: Ga " + to_string(tk->getFromStationID()) +
                                             " -> Ga " + to_string(tk->getToStationID()) + "\n";
                                ticket = ticket->next;
                            }
                            break;
                        }
                        case 15: {
                            activeViewMode = 0;
                            string code, strStID, strArr, strDep;
                            if (showInputDialog("THEM LICH DUNG GA", "1/4. Nhap Ma Tau (VD: SE1):", code, font) &&
                                showInputDialog("THEM LICH DUNG GA", "2/4. Nhap ID Ga dung (VD: 104):", strStID, font) &&
                                showInputDialog("THEM LICH DUNG GA", "3/4. Phut den ga trong ngay (VD: 360 cho 06:00):", strArr, font) &&
                                showInputDialog("THEM LICH DUNG GA", "4/4. Phut roi ga trong ngay (VD: 375 cho 06:15):", strDep, font)) 
                            {
                                try {
                                    if (!trainList.findTrainByCode(code) || !stationList.findStationByID(stoi(strStID))) {
                                        outputLog = "-> [LOI] Ma tau hoac ID Ga khong ton tai!";
                                    } else {
                                        trainList.addStopScheduleToTrain(code, stoi(strStID), stoi(strArr), stoi(strDep), 1);
                                        trainList.resolveConflictAndScheduleSiding(&stationList);
                                        FileManager::saveTrains("trains.txt", trainList);
                                        outputLog = "-> [THANH CONG] Da them lich dung tai Ga " + strStID + " cho tau " + code + " va tu dong kiem tra xung dot!";
                                    }
                                } catch (...) { outputLog = "-> [LOI] Du lieu thoi gian phai la so nguyen!"; }
                            }
                            break;
                        }
                        case 7: {
                            activeViewMode = 7; // Vẽ Bảng Vé
                            break;
                        }
                        case 8: {
                            activeViewMode = 0;
                            string tID, trCode, strCNo, strSNo, strFrom, strTo;
                            if (showInputDialog("SEGMENT BOOKING", "1/6. Nhap Ma Ve (VD: TCK201):", tID, font) &&
                                showInputDialog("SEGMENT BOOKING", "2/6. Nhap Ma Tau (VD: SE1):", trCode, font) &&
                                showInputDialog("SEGMENT BOOKING", "3/6. Nhap So Toa (VD: 1):", strCNo, font) &&
                                showInputDialog("SEGMENT BOOKING", "4/6. Nhap So Ghe (VD: 5):", strSNo, font) &&
                                showInputDialog("SEGMENT BOOKING", "5/6. Nhap ID Ga Di (VD: 101):", strFrom, font) &&
                                showInputDialog("SEGMENT BOOKING", "6/6. Nhap ID Ga Den (VD: 103):", strTo, font)) 
                            {
                                try {
                                    int cNo = stoi(strCNo), sNo = stoi(strSNo), fromSt = stoi(strFrom), toSt = stoi(strTo);
                                    if (ticketList.findTicketByID(tID) != nullptr) {
                                        outputLog = "-> [THAT BAI] Ma ve " + tID + " da ton tai!";
                                    } else if (ticketList.isSeatAvailable(&stationList, &trainList, trCode, cNo, sNo, fromSt, toSt)) {
                                        ticketList.addTicket(tID, trCode, cNo, sNo, fromSt, toSt);
                                        FileManager::saveTickets("tickets.txt", ticketList);
                                        outputLog = "-> [THANH CONG] Da dat ve " + tID + " (Tau " + trCode + ", Toa " + strCNo + ", Ghe " + strSNo +
                                                    ") chang Ga " + strFrom + " -> Ga " + strTo;
                                    } else {
                                        outputLog = "-> [THAT BAI] Ghe da co nguoi dat trong phan doan Km nay hoac sai chieu chay!";
                                    }
                                } catch (...) { outputLog = "-> [LOI] Du lieu nhap khong hop le!"; }
                            }
                            break;
                        }
                        case 16: { // Bổ sung tính năng findTicketByID có sẵn trong TicketList
                            string tID;
                            if (showInputDialog("TRA CUU VE", "Nhap Ma Ve can tim (VD: TCK101):", tID, font)) {
                                TicketNode* tk = ticketList.findTicketByID(tID);
                                if (tk) {
                                    outputLog = "=== KET QUA TRA CUU VE ===\n\n"
                                                " * Ma Ve        : " + tk->getTicketID() + "\n"
                                                " * Doan Tau     : " + tk->getTrainCode() + "\n"
                                                " * Vi tri       : Toa so " + to_string(tk->getCarriageNo()) + " - Ghe so " + to_string(tk->getSeatNo()) + "\n"
                                                " * Chang hanh khach : Ga ID " + to_string(tk->getFromStationID()) + " -> Ga ID " + to_string(tk->getToStationID()) + "\n";
                                } else {
                                    outputLog = "-> [KHONG TIM THAY] Khong ton tai ve co ma: " + tID;
                                }
                            }
                            break;
                        }
                        case 9: {
                            activeViewMode = 0;
                            string tID;
                            if (showInputDialog("HUY VE", "Nhap Ma Ve can huy (VD: TCK101):", tID, font)) {
                                if (ticketList.cancelTicket(tID)) {
                                    FileManager::saveTickets("tickets.txt", ticketList);
                                    outputLog = "-> [THANH CONG] Da huy ve " + tID + " va giai phong ghe tren phan doan!";
                                } else outputLog = "-> [LOI] Khong tim thay ma ve " + tID;
                            }
                            break;
                        }
                        case 10: {
                            showSeatMapUI(stationList, trainList, ticketList, font, outputLog);
                            break;
                        }
                        case 11: {
                            applyDelayUI(stationList, trainList, font, outputLog);
                            break;
                        }
                        case 12: {
                            GraphicalSimulationEngine::runSFMLSimulation(&stationList, &trainList);
                            outputLog = "-> Da dong cua so Mo phong 2D Running Engine va quay lai Trung tam Dieu hanh.";
                            break;
                        }
                        case 17: {
                            trainList.resolveConflictAndScheduleSiding(&stationList);
                            FileManager::saveTrains("trains.txt", trainList);
                            outputLog = "=== HOAN TAT QUET XUNG DOT & TAI LAP LICH (EVENT-DRIVEN) ===\n\n";
                            if (trainList.getLogCount() == 0) {
                                outputLog += " -> Toan tuyen khong phat sinh xung dot moi.\n";
                            } else {
                                for (int i = 0; i < trainList.getLogCount(); ++i) {
                                    outputLog += " >> " + trainList.getLog(i) + "\n";
                                }
                            }
                            break;
                        }
                        case 13: {
                            // Xóa sạch danh sách cũ trước khi nạp lại để tránh nhân đôi phần tử
                            stationList.clear();
                            trainList.clear();
                            ticketList.clear();
                            FileManager::loadStations("stations.txt", stationList);
                            FileManager::loadTrains("trains.txt", trainList);
                            FileManager::loadTickets("tickets.txt", ticketList);
                            trainList.resolveConflictAndScheduleSiding(&stationList);
                            outputLog = "-> [RELOAD THANH CONG] Da lam moi toan bo du lieu tu File Text va cap nhat lich tranh tau!";
                            break;
                        }
                        }
                    }
                }
            }
        }

        // ==================== VẼ GIAO DIỆN DASHBOARD HIỆN ĐẠI ====================
        window.clear(GuiTheme::BgMain);

        // 1. Đếm thống kê thời gian thực cho các thẻ KPI trên cùng
        const int stCount = stationList.getSize();
        const int trCount = trainList.getSize();
        const int tkCount = ticketList.getSize();

        auto drawKPICard = [&](float x, float y, float w, float h, const string& title, const string& val, sf::Color accent) {
            sf::RectangleShape card(sf::Vector2f(w, h));
            card.setPosition(sf::Vector2f(x, y));
            card.setFillColor(GuiTheme::CardBg);
            card.setOutlineThickness(1.0f);
            card.setOutlineColor(GuiTheme::Border);
            window.draw(card);

            sf::RectangleShape topLine(sf::Vector2f(w, 3.5f));
            topLine.setPosition(sf::Vector2f(x, y));
            topLine.setFillColor(accent);
            window.draw(topLine);

            sf::Text lbl(font, title, 11);
            lbl.setFillColor(GuiTheme::TextMuted);
            lbl.setPosition(sf::Vector2f(x + 14.0f, y + 12.0f));
            window.draw(lbl);

            sf::Text vTxt(font, val, 20);
            vTxt.setFillColor(GuiTheme::TextLight);
            vTxt.setPosition(sf::Vector2f(x + 14.0f, y + 32.0f));
            window.draw(vTxt);
        };

        // Tiêu đề Hệ thống bên trái
        sf::Text appTitle(font, "RAILWAY CONTROL CENTER", 20);
        appTitle.setFillColor(GuiTheme::AccentGold);
        appTitle.setPosition(sf::Vector2f(startX, 24.0f));
        window.draw(appTitle);

        sf::Text appSub(font, "PBL2: Single-Track Meet & Pass Dispatcher", 12);
        appSub.setFillColor(GuiTheme::AccentCyan);
        appSub.setPosition(sf::Vector2f(startX, 52.0f));
        window.draw(appSub);

        // 4 Thẻ KPI bên phải
        drawKPICard(390.0f, 18.0f, 220.0f, 68.0f, "HA TANG NHA GA", to_string(stCount) + " Ga tren tuyen", GuiTheme::AccentGreen);
        drawKPICard(625.0f, 18.0f, 220.0f, 68.0f, "DOAN TAU DANG QUAN LY", to_string(trCount) + " Chuyen (P1-P3)", GuiTheme::AccentCyan);
        drawKPICard(860.0f, 18.0f, 220.0f, 68.0f, "VE DA PHAT HANH", to_string(tkCount) + " Ve hanh khach", GuiTheme::AccentGold);
        drawKPICard(1095.0f, 18.0f, 240.0f, 68.0f, "LOG GIAI TOA XUNG DOT", to_string(trainList.getLogCount()) + " Su kien gan nhat", GuiTheme::AccentPurple);

        // 2. Vẽ các nhóm Menu bên trái
        for (int g = 0; g < NUM_GROUPS; ++g) {
            menuGroups[g].mainCategoryBtn.draw(window);
            if (menuGroups[g].isExpanded) {
                for (int s = 0; s < menuGroups[g].subCount; ++s) {
                    menuGroups[g].subButtons[s].draw(window);
                }
            }
        }

        // 3. Khung Console chính (Có cắt vùng hiển thị sf::View để hỗ trợ cuộn chuột mượt mà)
        sf::RectangleShape outputBox(sf::Vector2f(945.0f, 445.0f));
        outputBox.setPosition(sf::Vector2f(390.0f, 105.0f));
        outputBox.setFillColor(GuiTheme::PanelBg);
        outputBox.setOutlineThickness(1.5f);
        outputBox.setOutlineColor(GuiTheme::Border);
        window.draw(outputBox);

        // Sử dụng sf::View để nội dung văn bản khi cuộn không bị tràn ra ngoài khung outputBox
        sf::View defaultView = window.getView();
        sf::View scrollView(sf::FloatRect({390.0f, 105.0f}, {945.0f, 445.0f}));
        scrollView.setViewport(sf::FloatRect(
            {390.0f / 1360.0f, 105.0f / 768.0f},
            {945.0f / 1360.0f, 445.0f / 768.0f}
        ));
        window.setView(scrollView);

        sf::Text logText(font, outputLog, 13);
        logText.setFillColor(GuiTheme::TextLight);
        logText.setPosition(sf::Vector2f(410.0f, 120.0f + scrollOffsetY));
        window.draw(logText);

        window.setView(defaultView);

        // 4. Khung Nhật ký Xung đột Thời gian thực (Circular Buffer Conflict Logs) ở góc dưới phải
        sf::RectangleShape conflictBox(sf::Vector2f(945.0f, 175.0f));
        conflictBox.setPosition(sf::Vector2f(390.0f, 568.0f));
        conflictBox.setFillColor(GuiTheme::PanelBg);
        conflictBox.setOutlineThickness(1.5f);
        conflictBox.setOutlineColor(GuiTheme::Border);
        window.draw(conflictBox);

        sf::Text cfTitle(font, "NHAT KY DIEU PHOI & TRANH TAU TU DONG (CIRCULAR BUFFER CONFLICT LOGS)", 12);
        cfTitle.setFillColor(GuiTheme::AccentPurple);
        cfTitle.setPosition(sf::Vector2f(408.0f, 578.0f));
        window.draw(cfTitle);

        if (trainList.getLogCount() == 0) {
            sf::Text noCf(font, "-> He thong chua ghi nhan xung dot nao hoac toan bo tuyen dang thong suot.", 12);
            noCf.setFillColor(GuiTheme::TextMuted);
            noCf.setPosition(sf::Vector2f(408.0f, 604.0f));
            window.draw(noCf);
        } else {
            float cy = 602.0f;
            int startIdx = std::max(0, trainList.getLogCount() - 6); // Hiển thị 6 dòng mới nhất trên Dashboard
            for (int i = startIdx; i < trainList.getLogCount(); ++i) {
                sf::Text cfLine(font, ">> " + trainList.getLog(i), 12);
                cfLine.setFillColor(GuiTheme::AccentGold);
                cfLine.setPosition(sf::Vector2f(408.0f, cy));
                window.draw(cfLine);
                cy += 21.0f;
            }
        }

        window.display();
    }
}