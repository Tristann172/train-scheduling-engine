#include "GuiMenu.h"
#include "StationList.h"
#include "TrainList.h"
#include "FileManager.h"
#include "GraphicalSimulationEngine.h"

#include "StationNode.h"
#include "TrainNode.h"
#include "CarriageNode.h"
#include "StopScheduleNode.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <fstream>

using namespace std;

// Hiển thị phút tuyệt đối theo ngày, không làm mất thông tin lịch nhiều ngày.
static string formatScheduleMinute(int totalMinutes) {
    const int day = totalMinutes / 1440;
    const int minuteOfDay = totalMinutes % 1440;
    stringstream formatted;
    if (day > 0) formatted << "D+" << day << " ";
    formatted << setfill('0') << setw(2) << minuteOfDay / 60 << ":"
              << setfill('0') << setw(2) << minuteOfDay % 60;
    return formatted.str();
}

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

// ==================== 2. HÀM RENDER ĐỒ HỌA BẢNG DỮ LIỆU (CUSTOM TABLE RENDERERS) ====================

// --- 2.1. VẼ BẢNG DANH SÁCH NHÀ GA (ACTION ID: 1) ---
void renderStationTable(sf::RenderWindow& window, sf::Font& font, const StationList& stationList, float startY, const string& filter) {
    const float tableX = 410.0f;
    const float colWidths[] = { 80.0f, 150.0f, 130.0f, 170.0f, 385.0f };
    const float rowHeight = 32.0f;
    const string headers[] = { "GA ID", "TEN GA", "LY TRINH", "SO RAY (MAX)", "NANG LUC HA TANG" };

    float currentX = tableX;
    for (int i = 0; i < 5; ++i) {
        sf::RectangleShape headerBox(sf::Vector2f(colWidths[i] - 2.0f, rowHeight));
        headerBox.setPosition(sf::Vector2f(currentX, startY));
        headerBox.setFillColor(GuiTheme::CardBg);
        headerBox.setOutlineThickness(1.0f);
        headerBox.setOutlineColor(GuiTheme::Border);
        window.draw(headerBox);

        sf::Text txt(font, headers[i], 12);
        txt.setFillColor(GuiTheme::AccentGold);
        txt.setPosition(sf::Vector2f(currentX + 10.0f, startY + 8.0f));
        window.draw(txt);
        currentX += colWidths[i];
    }

    float currentY = startY + rowHeight + 4.0f;
    int rowIndex = 0;

    for (LinkedList<StationNode*>::Node* node = stationList.getHead(); node != nullptr; node = node->next) {
        StationNode* st = node->data;
        if (!filter.empty() && to_string(st->getStationID()).find(filter) == string::npos) continue;
        currentX = tableX;
        sf::Color rowBg = (rowIndex % 2 == 0) ? sf::Color(28, 32, 46) : sf::Color(36, 41, 58);

        string cellData[5];
        cellData[0] = to_string(st->getStationID());
        cellData[1] = st->getName();
        cellData[2] = to_string(static_cast<int>(st->getKmMarker())) + " km";
        cellData[3] = to_string(st->getMaxTracks()) + " Ray";
        
        if (st->getMaxTracks() == 1) {
            cellData[4] = "Ga xep (1 ray - Khong the tranh tau)";
        } else if (st->getMaxTracks() >= 4) {
            cellData[4] = "Ga trung tam (" + to_string(st->getMaxTracks() - 1) + " ray phu)";
        } else {
            cellData[4] = "Co ray phu (" + to_string(st->getMaxTracks() - 1) + " ray phu)";
        }

        for (int c = 0; c < 5; ++c) {
            sf::RectangleShape cellBox(sf::Vector2f(colWidths[c] - 2.0f, rowHeight - 2.0f));
            cellBox.setPosition(sf::Vector2f(currentX, currentY));
            cellBox.setFillColor(rowBg);
            cellBox.setOutlineThickness(1.0f);
            cellBox.setOutlineColor(GuiTheme::Border);
            window.draw(cellBox);

            sf::Text cellTxt(font, cellData[c], 12);
            if (c == 3) {
                cellTxt.setFillColor(st->getMaxTracks() > 2 ? GuiTheme::AccentGreen : GuiTheme::AccentCyan);
            } else if (c == 4 && st->getMaxTracks() == 1) {
                cellTxt.setFillColor(GuiTheme::AccentRed);
            } else {
                cellTxt.setFillColor(GuiTheme::TextLight);
            }

            cellTxt.setPosition(sf::Vector2f(currentX + 10.0f, currentY + 7.0f));
            window.draw(cellTxt);
            currentX += colWidths[c];
        }
        currentY += rowHeight;
        rowIndex++;
    }
}

// --- 2.2. VẼ BẢNG DANH SÁCH ĐOÀN TÀU (ACTION ID: 4) ---
void renderTrainTable(sf::RenderWindow& window, sf::Font& font, const TrainList& trainList, float startY, const string& filter) {
    const float tableX = 410.0f;
    const float colWidths[] = { 120.0f, 260.0f, 180.0f, 355.0f };
    const float rowHeight = 36.0f;
    const string headers[] = { "MA TAU", "CAP UU TIEN (PRIORITY)", "HUONG CHAY", "TRANG THAI VAN HANH" };

    float currentX = tableX;
    for (int i = 0; i < 4; ++i) {
        sf::RectangleShape headerBox(sf::Vector2f(colWidths[i] - 2.0f, rowHeight));
        headerBox.setPosition(sf::Vector2f(currentX, startY));
        headerBox.setFillColor(GuiTheme::CardBg);
        headerBox.setOutlineThickness(1.0f);
        headerBox.setOutlineColor(GuiTheme::Border);
        window.draw(headerBox);

        sf::Text txt(font, headers[i], 12);
        txt.setFillColor(GuiTheme::AccentGold);
        txt.setPosition(sf::Vector2f(currentX + 12.0f, startY + 10.0f));
        window.draw(txt);
        currentX += colWidths[i];
    }

    float currentY = startY + rowHeight + 4.0f;
    int rowIndex = 0;

    for (LinkedList<TrainNode*>::Node* node = trainList.getHead(); node != nullptr; node = node->next) {
        TrainNode* tr = node->data;
        if (!filter.empty() && tr->getTrainCode().find(filter) == string::npos) continue;
        currentX = tableX;
        sf::Color rowBg = (rowIndex % 2 == 0) ? sf::Color(28, 32, 46) : sf::Color(36, 41, 58);

        string cellData[4];
        cellData[0] = tr->getTrainCode();
        cellData[1] = (tr->getPriority() == 1) ? "P1 - Tau Khach Nhanh (SE)" : 
                      ((tr->getPriority() == 2) ? "P2 - Tau Khach Thuong (TN)" : "P3 - Tau Hang");
        cellData[2] = (tr->getDirection() == 1) ? "Bac -> Nam (1)" : "Nam -> Bac (0)";
        cellData[3] = "Hoat dong binh thuong";

        for (int c = 0; c < 4; ++c) {
            sf::RectangleShape cellBox(sf::Vector2f(colWidths[c] - 2.0f, rowHeight - 2.0f));
            cellBox.setPosition(sf::Vector2f(currentX, currentY));
            cellBox.setFillColor(rowBg);
            cellBox.setOutlineThickness(1.0f);
            cellBox.setOutlineColor(GuiTheme::Border);
            window.draw(cellBox);

            sf::Text cellTxt(font, cellData[c], 12);
            if (c == 0) cellTxt.setFillColor(GuiTheme::AccentCyan);
            else if (c == 1) cellTxt.setFillColor(tr->getPriority() == 1 ? GuiTheme::AccentGreen : GuiTheme::AccentGold);
            else cellTxt.setFillColor(GuiTheme::TextLight);

            cellTxt.setPosition(sf::Vector2f(currentX + 12.0f, currentY + 9.0f));
            window.draw(cellTxt);
            currentX += colWidths[c];
        }
        currentY += rowHeight;
        rowIndex++;
    }
}

// --- 2.3. VẼ THẺ TOA XE & BẢNG LỊCH TRÌNH DỪNG GA (ACTION ID: 5) ---
void renderTrainDetailsPanel(sf::RenderWindow& window, sf::Font& font, const TrainList& trainList, const StationList& stationList, float startY, const string& filter) {
    float currentY = startY;
    const float panelX = 410.0f;
    const float panelWidth = 915.0f;

    for (LinkedList<TrainNode*>::Node* trainNode = trainList.getHead(); trainNode != nullptr; trainNode = trainNode->next) {
        TrainNode* tr = trainNode->data;
        if (!filter.empty()) {
            bool matchesStation = false;
            for (LinkedList<StopScheduleNode*>::Node* stop = tr->getHeadSchedule();
                 stop; stop = stop->next) {
                if (to_string(stop->data->getStationID()).find(filter) != string::npos) {
                    matchesStation = true;
                    break;
                }
            }
            if (!matchesStation) continue;
        }

        // 1. Thẻ tiêu đề Tàu
        sf::RectangleShape trainCard(sf::Vector2f(panelWidth, 40.0f));
        trainCard.setPosition(sf::Vector2f(panelX, currentY));
        trainCard.setFillColor(GuiTheme::CardBg);
        trainCard.setOutlineThickness(1.0f);
        trainCard.setOutlineColor(GuiTheme::AccentCyan);
        window.draw(trainCard);

        string titleStr = "TAU " + tr->getTrainCode() + " | Uu tien: P" + to_string(tr->getPriority()) +
                         " | Huong: " + (tr->getDirection() == 1 ? "Bac -> Nam" : "Nam -> Bac");
        sf::Text titleTxt(font, titleStr, 13);
        titleTxt.setFillColor(GuiTheme::AccentGold);
        titleTxt.setPosition(sf::Vector2f(panelX + 15.0f, currentY + 11.0f));
        window.draw(titleTxt);
        currentY += 46.0f;

        // 2. Trực quan hóa các Toa xe dạng khối ghép (Train Coaches)
        sf::Text carriageLabel(font, "Toa xe:", 12);
        carriageLabel.setFillColor(GuiTheme::TextMuted);
        carriageLabel.setPosition(sf::Vector2f(panelX + 10.0f, currentY + 5.0f));
        window.draw(carriageLabel);

        float coachX = panelX + 70.0f;
        for (LinkedList<CarriageNode*>::Node* cNode = tr->getHeadCarriage(); cNode != nullptr; cNode = cNode->next) {
            CarriageNode* c = cNode->data;

            sf::RectangleShape coachBox(sf::Vector2f(140.0f, 28.0f));
            coachBox.setPosition(sf::Vector2f(coachX, currentY));
            coachBox.setFillColor(sf::Color(45, 52, 74));
            coachBox.setOutlineThickness(1.0f);
            coachBox.setOutlineColor(GuiTheme::Border);
            window.draw(coachBox);

            string cStr = "Toa " + to_string(c->getCarriageNo()) + ": " + c->getType();
            sf::Text cTxt(font, cStr, 10);
            cTxt.setFillColor(GuiTheme::TextLight);
            cTxt.setPosition(sf::Vector2f(coachX + 6.0f, currentY + 7.0f));
            window.draw(cTxt);

            coachX += 146.0f;
        }
        currentY += 38.0f;

        // 3. Bảng Lịch trình đỗ Ga bên trong Thẻ
        const float subColW[] = { 80.0f, 180.0f, 150.0f, 150.0f, 355.0f };
        const string subHeaders[] = { "GA ID", "TEN GA", "GIO DEN", "GIO DI", "DUONG RAY SU DUNG" };

        float subX = panelX;
        for (int i = 0; i < 5; ++i) {
            sf::RectangleShape sb(sf::Vector2f(subColW[i] - 2.0f, 26.0f));
            sb.setPosition(sf::Vector2f(subX, currentY));
            sb.setFillColor(sf::Color(22, 25, 36));
            sb.setOutlineThickness(1.0f);
            sb.setOutlineColor(GuiTheme::Border);
            window.draw(sb);

            sf::Text stxt(font, subHeaders[i], 11);
            stxt.setFillColor(GuiTheme::AccentCyan);
            stxt.setPosition(sf::Vector2f(subX + 8.0f, currentY + 5.0f));
            window.draw(stxt);
            subX += subColW[i];
        }
        currentY += 28.0f;

        for (LinkedList<StopScheduleNode*>::Node* sNode = tr->getHeadSchedule(); sNode != nullptr; sNode = sNode->next) {
            StopScheduleNode* s = sNode->data;
            if (!filter.empty() &&
                to_string(s->getStationID()).find(filter) == string::npos) continue;
            StationNode* st = stationList.findStationByID(s->getStationID());
            subX = panelX;

            string rowVal[5] = {
                to_string(s->getStationID()),
                st ? st->getName() : "Ga " + to_string(s->getStationID()),
                formatScheduleMinute(s->getArrivalMinute()),
                formatScheduleMinute(s->getDepartureMinute()),
                (s->getTrackUsed() > 1)
                    ? "Ray " + to_string(s->getTrackUsed()) +
                      (sNode->next == nullptr ? " (ray phu dau cuoi)" : " (ray phu)")
                    : "Ray 1 (Ray chinh)"
            };

            for (int i = 0; i < 5; ++i) {
                sf::RectangleShape cell(sf::Vector2f(subColW[i] - 2.0f, 24.0f));
                cell.setPosition(sf::Vector2f(subX, currentY));
                cell.setFillColor(GuiTheme::PanelBg);
                cell.setOutlineThickness(1.0f);
                cell.setOutlineColor(GuiTheme::Border);
                window.draw(cell);

                sf::Text rtxt(font, rowVal[i], 11);
                if (i == 4 && s->getTrackUsed() > 1) rtxt.setFillColor(GuiTheme::AccentPurple);
                else rtxt.setFillColor(GuiTheme::TextLight);

                rtxt.setPosition(sf::Vector2f(subX + 8.0f, currentY + 4.0f));
                window.draw(rtxt);
                subX += subColW[i];
            }
            currentY += 25.0f;
        }
        currentY += 20.0f;
    }
}

// ==================== 4. GIAO DIỆN LAN TRUYỀN TRỄ & TÁI LẬP LỊCH ====================
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

// Ghi thao tác bảo trì vào lịch sử lệnh để trạng thái được giữ sau khi khởi động lại.
static bool changeMaintenanceUI(bool opening, StationList& stationList, TrainList& trainList,
                                sf::Font& font, string& outputLog) {
    string resource, firstText, secondText, startText, endText;
    if (!GuiMenu::showInputDialog(opening ? "MO TAI NGUYEN" : "DONG TAI NGUYEN",
                                  "Loai tai nguyen (STATION, BLOCK, TRACK):",
                                  resource, font)) return false;
    transform(resource.begin(), resource.end(), resource.begin(),
              [](unsigned char ch) { return static_cast<char>(toupper(ch)); });
    if (resource != "STATION" && resource != "BLOCK" && resource != "TRACK") {
        outputLog = "-> [TU CHOI] Loai tai nguyen phai la STATION, BLOCK hoac TRACK.";
        return false;
    }

    if (!GuiMenu::showInputDialog("BAO TRI", "ID ga (hoac ga dau cua khu gian):",
                                  firstText, font)) return false;
    const bool isStation = resource == "STATION";
    if (!isStation && !GuiMenu::showInputDialog("BAO TRI",
            resource == "BLOCK" ? "ID ga ke tiep cua khu gian:" : "So ray tai ga:",
            secondText, font)) return false;
    if (!GuiMenu::showInputDialog("BAO TRI", "Phut bat dau (so phut tu moc ngay 0):",
                                  startText, font) ||
        !GuiMenu::showInputDialog("BAO TRI", "Phut ket thuc:", endText, font)) return false;

    try {
        size_t used = 0;
        const int first = stoi(firstText, &used);
        if (used != firstText.size()) throw invalid_argument("station");
        const int second = isStation ? 0 : stoi(secondText, &used);
        if (!isStation && used != secondText.size()) throw invalid_argument("resource");
        const int start = stoi(startText, &used);
        if (used != startText.size()) throw invalid_argument("start");
        const int end = stoi(endText, &used);
        if (used != endText.size()) throw invalid_argument("end");

        StationNode* firstStation = stationList.findStationByID(first);
        StationNode* secondStation = resource == "BLOCK"
            ? stationList.findStationByID(second) : nullptr;
        bool validResource = firstStation != nullptr;
        if (resource == "BLOCK") {
            validResource = validResource && secondStation && first != second &&
                firstStation->getKmMarker() != secondStation->getKmMarker();
            if (validResource) {
                const double lowKm = min(firstStation->getKmMarker(), secondStation->getKmMarker());
                const double highKm = max(firstStation->getKmMarker(), secondStation->getKmMarker());
                for (LinkedList<StationNode*>::Node* station = stationList.getHead();
                     station; station = station->next) {
                    if (station->data->getKmMarker() > lowKm &&
                        station->data->getKmMarker() < highKm) validResource = false;
                }
            }
        } else if (resource == "TRACK") {
            validResource = validResource && second >= 1 &&
                            second <= firstStation->getMaxTracks();
        }
        if (!validResource || start < 0 || end <= start) {
            outputLog = "-> [TU CHOI] Tai nguyen khong ton tai/khong hop le hoac khoang thoi gian sai.";
            return false;
        }

        const bool changed = opening
            ? trainList.removeMaintenance(resource, first, second, start, end)
            : trainList.addMaintenance(resource, first, second, start, end);
        if (!changed) {
            outputLog = "-> [TU CHOI] Khong tim thay cua so bao tri can mo.";
            return false;
        }

        trainList.resolveConflictAndScheduleSiding(&stationList);
        if (!FileManager::saveTrains("trains.txt", trainList)) {
            if (opening) trainList.addMaintenance(resource, first, second, start, end);
            else trainList.removeMaintenance(resource, first, second, start, end);
            trainList.resolveConflictAndScheduleSiding(&stationList);
            outputLog = "-> [LOI] Khong ghi duoc trains.txt; da khoi phuc trang thai bao tri.";
            return false;
        }

        const string operation = opening ? "OPEN_" : "CLOSE_";
        const string line = operation + resource + "|" + to_string(first) +
            (isStation ? "" : "|" + to_string(second)) + "|" +
            to_string(start) + "|" + to_string(end);
        ofstream commands("commands.txt", ios::app);
        if (commands) commands << line << '\n';
        commands.flush();
        if (!commands) {
            if (opening) trainList.addMaintenance(resource, first, second, start, end);
            else trainList.removeMaintenance(resource, first, second, start, end);
            trainList.resolveConflictAndScheduleSiding(&stationList);
            FileManager::saveTrains("trains.txt", trainList);
            outputLog = "-> [LOI] Khong ghi duoc commands.txt; da khoi phuc trang thai bao tri.";
            return false;
        }
        outputLog = opening
            ? "-> [THANH CONG] Da mo tai nguyen va cap nhat lai dieu phoi."
            : "-> [THANH CONG] Da dong tai nguyen, cap nhat dieu phoi va luu commands.txt.";
        return true;
    } catch (const exception&) {
        outputLog = "-> [LOI] ID va thoi gian phai la so nguyen hop le.";
        return false;
    }
}

// ==================== 5. MAIN DASHBOARD CONTROL CENTER ====================
void GuiMenu::runGUIMenu(StationList& stationList, TrainList& trainList) {
    sf::RenderWindow window(sf::VideoMode({1360, 768}), "PBL2: TRUNG TAM DIEU HANH & QUAN LY DUONG SAT DON (CONTROL CENTER)");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("arial.ttf")) {
        cout << "[Loi] Khong tim thay font 'arial.ttf'!\n";
    }

    trainList.resolveConflictAndScheduleSiding(&stationList);

    const float startX = 25.0f, startY = 105.0f, btnW = 340.0f, mainH = 40.0f, subH = 31.0f, gapY = 7.0f;
    static const int NUM_GROUPS = 5;
    MenuGroup menuGroups[NUM_GROUPS];

    // 1. Nhóm Quản lý Hạ tầng Ga
    menuGroups[0].initGroup(startX, 0, btnW, mainH, "[1] QUAN LY HA TANG GA (MAX TRACKS)", font, 100);
    menuGroups[0].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "1.1. Danh sach Ga & Nang luc Ray (maxTracks)", font, 1);
    menuGroups[0].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "1.2. Them Ga moi vao tuyen", font, 2);
    menuGroups[0].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "1.3. Tra cuu chi tiet Ga theo ID", font, 3);
    menuGroups[0].isExpanded = true;
    menuGroups[0].mainCategoryBtn.setSelected(true);

    // 2. Nhóm Quản lý Đoàn tàu, Toa xe & Lịch trình
    menuGroups[1].initGroup(startX, 0, btnW, mainH, "[2] QUAN LY TAU, TOA XE & LICH TRINH", font, 200);
    menuGroups[1].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "2.1. Danh sach Doan tau & Cap uu tien (P1-P3)", font, 4);
    menuGroups[1].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "2.2. Bang chi tiet Toa xe & Lich dung", font, 5);
    menuGroups[1].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "2.3. Them Doan tau moi (kem Priority)", font, 6);
    menuGroups[1].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "2.4. Gan Toa xe moi vao Doan tau", font, 14);
    menuGroups[1].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "2.5. Them Diem dung (Stop Schedule) cho Tau", font, 15);
    menuGroups[1].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "2.6. Bai bo chuyen tau", font, 19);
    menuGroups[1].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "2.7. Bai bo diem dung", font, 20);
    menuGroups[1].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "2.8. Dieu chinh lich dung tai ga", font, 22);

    // 3. Nhóm đóng/mở tài nguyên khi bảo trì và báo cáo sự cố.
    menuGroups[2].initGroup(startX, 0, btnW, mainH, "[3] BAO TRI & SU CO", font, 300);
    menuGroups[2].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "3.1. Dong tai nguyen do bao tri", font, 23);
    menuGroups[2].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "3.2. Mo tai nguyen sau bao tri", font, 24);
    menuGroups[2].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "3.3. Bao cao su co tre gio (Delay)", font, 11);

    // 4. Nhóm Điều phối Xung đột & Mô phỏng Đồ họa.
    menuGroups[3].initGroup(startX, 0, btnW, mainH, "[4] DIEU PHOI & MO PHONG 2D", font, 400);
    menuGroups[3].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "4.1. Chay Mo phong Do hoa SFML 2D Engine", font, 12);
    menuGroups[3].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "4.2. Quet Xung dot & Phan luong Ray phu ngay", font, 17);
    menuGroups[3].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "4.3. Nap lai du lieu goc tu File Text", font, 13);
    menuGroups[3].addSubButton(startX + 15.0f, 0, btnW - 15.0f, subH, "4.4. Ap dung lenh quan ly tu commands.txt", font, 18);

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
    int activeViewMode = 1; // Mặc định mở Bảng Ga khi khởi động
    string stationSearch, trainSearch, stopStationSearch;
    int searchFocus = 0;

    while (window.isOpen()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        bool mouseClicked = false;

        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseBtn->button == sf::Mouse::Button::Left) mouseClicked = true;
            }
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
            if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()) {
                string* target = searchFocus == 1 ? &stationSearch :
                    (searchFocus == 2 ? &trainSearch :
                     (searchFocus == 3 ? &stopStationSearch : nullptr));
                if (target) {
                    if (textEntered->unicode == 8) {
                        if (!target->empty()) target->pop_back();
                    } else if (target->size() < 32 &&
                               textEntered->unicode >= 32 && textEntered->unicode <= 126) {
                        target->push_back(static_cast<char>(textEntered->unicode));
                    }
                }
            }
        }
        if (mouseClicked) {
            const sf::FloatRect searchBounds({1055.0f, 112.0f}, {265.0f, 28.0f});
            const sf::Vector2f pointer(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
            if (searchBounds.contains(pointer)) {
                searchFocus = activeViewMode == 1 ? 1 :
                    (activeViewMode == 4 ? 2 : (activeViewMode == 5 ? 3 : 0));
            } else {
                searchFocus = 0;
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
                    for (int k = 0; k < NUM_GROUPS; ++k) {
                        menuGroups[k].isExpanded = false;
                        menuGroups[k].mainCategoryBtn.setSelected(false);
                    }
                    menuGroups[g].isExpanded = willExpand;
                    menuGroups[g].mainCategoryBtn.setSelected(willExpand);
                }
                continue;
            }

            if (menuGroups[g].isExpanded) {
                for (int s = 0; s < menuGroups[g].subCount; ++s) {
                    GuiButton& subBtn = menuGroups[g].subButtons[s];
                    subBtn.updateHover(mousePos);

                    if (subBtn.isClicked(mouseClicked)) {
                        scrollOffsetY = 0.0f;

                        switch (subBtn.actionID) {
                        case 1: {
                            activeViewMode = 1; // Vẽ Bảng Nhà Ga
                            break;
                        }
                        case 18: {
                            activeViewMode = 0;
                            string commandLog;
                            if (FileManager::processInputCommands("commands.txt", stationList,
                                    trainList, commandLog)) {
                                FileManager::saveStations("stations.txt", stationList);
                                FileManager::saveTrains("trains.txt", trainList);
                                outputLog = "=== KET QUA LENH commands.txt ===\n\n" + commandLog;
                            } else {
                                outputLog = "-> [LOI] " + commandLog;
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
                                    if (stationList.addStation(stoi(strID), name, stod(strKm), stoi(strTracks))) {
                                        FileManager::saveStations("stations.txt", stationList);
                                        outputLog = "-> [THANH CONG] Da them Ga " + name + " (maxTracks = " + strTracks + ") va luu file!";
                                    } else outputLog = "-> [TU CHOI] ID, ten, moc km, so ray khong hop le hoac ID da ton tai.";
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
                            activeViewMode = 4; // Vẽ Bảng Đoàn Tàu
                            break;
                        }
                        case 5: {
                            activeViewMode = 5; // Vẽ Thẻ Toa xe & Lịch trình
                            break;
                        }
                        case 6: {
                            activeViewMode = 0;
                            string code, strDir, strPrio;
                            if (showInputDialog("THEM TAU MOI", "1/3. Nhap Ma Tau (VD: SE5, TN3, HH1):", code, font) &&
                                showInputDialog("THEM TAU MOI", "2/3. Huong Chay (1: Bac->Nam, 0: Nam->Bac):", strDir, font) &&
                                showInputDialog("THEM TAU MOI", "3/3. Cap Uu Tien (1: SE, 2: TN, 3: Tau Hang, 0: Tu suy luan):", strPrio, font)) 
                            {
                                try {
                                    int prio = stoi(strPrio);
                                    if (trainList.addTrain(code, stoi(strDir), prio <= 0 ? -1 : prio)) {
                                        FileManager::saveTrains("trains.txt", trainList);
                                        TrainNode* added = trainList.findTrainByCode(code);
                                        outputLog = "-> [THANH CONG] Da them Tau " + code + " voi Cap uu tien P" +
                                                    to_string(added ? added->getPriority() : 1) + "!";
                                    } else outputLog = "-> [TU CHOI] Ma tau da ton tai hoac huong chay khong hop le.";
                                } catch (...) { outputLog = "-> [LOI] Du lieu nhap vao khong hop le!"; }
                            }
                            break;
                        }
                        case 14: {
                            activeViewMode = 0;
                            string code, strCNo, type;
                            if (showInputDialog("GAN TOA XE VAO TAU", "1/3. Nhap Ma Tau (VD: SE1):", code, font) &&
                                showInputDialog("GAN TOA XE VAO TAU", "2/3. Nhap So thu tu Toa (VD: 3):", strCNo, font) &&
                                showInputDialog("GAN TOA XE VAO TAU", "3/3. Loai Toa (VD: NgoiMem, GiuongNam):", type, font))
                            {
                                try {
                                    if (!trainList.findTrainByCode(code)) {
                                        outputLog = "-> [LOI] Khong tim thay doan tau " + code + "!";
                                    } else {
                                        if (trainList.addCarriageToTrain(code, stoi(strCNo), type)) {
                                            FileManager::saveTrains("trains.txt", trainList);
                                            outputLog = "-> [THANH CONG] Da gan Toa so " + strCNo + " (" + type + ") vao tau " + code + "!";
                                        } else outputLog = "-> [TU CHOI] So toa, loai toa khong hop le hoac so toa da ton tai.";
                                    }
                                } catch (...) { outputLog = "-> [LOI] So toa phai la so nguyen!"; }
                            }
                            break;
                        }
                        case 15: {
                            activeViewMode = 0;
                            string code, strStID, strArr, strDep;
                            if (showInputDialog("THEM LICH DUNG GA", "1/4. Nhap Ma Tau (VD: SE1):", code, font) &&
                                showInputDialog("THEM LICH DUNG GA", "2/4. Nhap ID Ga dung (VD: 104):", strStID, font) &&
                                showInputDialog("THEM LICH DUNG GA", "3/4. Phut tu moc ngay 0 (1500 = D+1 01:00):", strArr, font) &&
                                showInputDialog("THEM LICH DUNG GA", "4/4. Phut roi ga tu moc ngay 0:", strDep, font))
                            {
                                try {
                                    if (!trainList.addStopScheduleToTrain(code, stoi(strStID), stoi(strArr),
                                                                          stoi(strDep), 1, &stationList)) {
                                        outputLog = "-> [TU CHOI] Tau/ga khong ton tai, gio sai thu tu, ga trung lap hoac huong tuyen khong hop le.";
                                    } else {
                                        FileManager::saveTrains("trains.txt", trainList);
                                        outputLog = "-> [THANH CONG] Da them lich dung tai Ga " + strStID + " cho tau " + code + " va tu dong kiem tra xung dot!";
                                    }
                                } catch (...) { outputLog = "-> [LOI] Du lieu thoi gian phai la so nguyen!"; }
                            }
                            break;
                        }
                        case 19: {
                            activeViewMode = 0;
                            string code;
                            if (showInputDialog("BAI BO CHUYEN TAU", "Nhap ma tau can bai bo:", code, font)) {
                                if (trainList.removeTrain(code)) {
                                    trainList.resolveConflictAndScheduleSiding(&stationList);
                                    if (FileManager::saveTrains("trains.txt", trainList))
                                        outputLog = "-> [THANH CONG] Da bai bo tau " + code + " va cap nhat dieu phoi.";
                                    else outputLog = "-> [LOI] Da xoa tau nhung khong ghi duoc trains.txt.";
                                } else outputLog = "-> [TU CHOI] Khong tim thay tau " + code + ".";
                            }
                            break;
                        }
                        case 20: {
                            activeViewMode = 0;
                            string code, stationID;
                            if (showInputDialog("BAI BO DIEM DUNG", "Nhap ma tau:", code, font) &&
                                showInputDialog("BAI BO DIEM DUNG", "Nhap ID ga dung can xoa:", stationID, font)) {
                                try {
                                    const int id = stoi(stationID);
                                    if (trainList.removeStopSchedule(code, id)) {
                                        trainList.resolveConflictAndScheduleSiding(&stationList);
                                        if (FileManager::saveTrains("trains.txt", trainList))
                                            outputLog = "-> [THANH CONG] Da xoa diem dung tai ga " + stationID +
                                                        " va cap nhat dieu phoi.";
                                        else outputLog = "-> [LOI] Da xoa diem dung nhung khong ghi duoc trains.txt.";
                                    } else outputLog = "-> [TU CHOI] Lich khong hop le hoac tau khong dung tai ga " + stationID + ".";
                                } catch (const exception&) {
                                    outputLog = "-> [LOI] ID ga phai la so nguyen.";
                                }
                            }
                            break;
                        }
                        case 22: {
                            activeViewMode = 0;
                            string code, stationID, arrival, departure, track;
                            if (showInputDialog("DIEU CHINH LICH DUNG", "Nhap ma tau:", code, font) &&
                                showInputDialog("DIEU CHINH LICH DUNG", "Nhap ID ga dung:", stationID, font) &&
                                showInputDialog("DIEU CHINH LICH DUNG", "Phut den (so phut tu moc ngay 0):", arrival, font) &&
                                showInputDialog("DIEU CHINH LICH DUNG", "Phut roi:", departure, font) &&
                                showInputDialog("DIEU CHINH LICH DUNG", "Ray su dung (1 la ray chinh):", track, font)) {
                                try {
                                    if (trainList.updateStopSchedule(code, stoi(stationID), stoi(arrival),
                                            stoi(departure), stoi(track), &stationList)) {
                                        trainList.resolveConflictAndScheduleSiding(&stationList);
                                        if (FileManager::saveTrains("trains.txt", trainList))
                                            outputLog = "-> [THANH CONG] Da dieu chinh lich dung va cap nhat dieu phoi.";
                                        else outputLog = "-> [LOI] Da cap nhat lich nhung khong ghi duoc trains.txt.";
                                    } else outputLog = "-> [TU CHOI] Thoi gian/ray sai hoac vi pham thu tu tuyen.";
                                } catch (const exception&) {
                                    outputLog = "-> [LOI] ID, thoi gian va ray phai la so nguyen.";
                                }
                            }
                            break;
                        }
                        case 11: {
                            activeViewMode = 0;
                            applyDelayUI(stationList, trainList, font, outputLog);
                            break;
                        }
                        case 23: {
                            activeViewMode = 0;
                            changeMaintenanceUI(false, stationList, trainList, font, outputLog);
                            break;
                        }
                        case 24: {
                            activeViewMode = 0;
                            changeMaintenanceUI(true, stationList, trainList, font, outputLog);
                            break;
                        }
                        case 12: {
                            activeViewMode = 0;
                            GraphicalSimulationEngine::runSFMLSimulation(&stationList, &trainList);
                            outputLog = "-> Da dong cua so Mo phong 2D Running Engine va quay lai Trung tam Dieu hanh.";
                            break;
                        }
                        case 17: {
                            activeViewMode = 0;
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
                            activeViewMode = 0;
                            stationList.clear();
                            trainList.clear();
                            FileManager::loadStations("stations.txt", stationList);
                            FileManager::loadTrains("trains.txt", trainList);
                            string commandLog;
                            if (FileManager::processInputCommands("commands.txt", stationList,
                                    trainList, commandLog)) {
                                outputLog = "-> [RELOAD THANH CONG] Da nap du lieu va commands.txt.\n\n" +
                                            commandLog;
                            } else {
                                trainList.resolveConflictAndScheduleSiding(&stationList);
                                outputLog = "-> [LOI] " + commandLog;
                            }
                            break;
                        }
                        }
                    }
                }
            }
        }

        // ==================== VẼ GIAO DIỆN DASHBOARD HIỆN ĐẠI ====================
        window.clear(GuiTheme::BgMain);

        const int stCount = stationList.getSize();
        const int trCount = trainList.getSize();

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

        sf::Text appTitle(font, "RAILWAY CONTROL CENTER", 20);
        appTitle.setFillColor(GuiTheme::AccentGold);
        appTitle.setPosition(sf::Vector2f(startX, 24.0f));
        window.draw(appTitle);

        sf::Text appSub(font, "PBL2: Single-Track Meet & Pass Dispatcher", 12);
        appSub.setFillColor(GuiTheme::AccentCyan);
        appSub.setPosition(sf::Vector2f(startX, 52.0f));
        window.draw(appSub);

        drawKPICard(390.0f, 18.0f, 220.0f, 68.0f, "HA TANG NHA GA", to_string(stCount) + " Ga tren tuyen", GuiTheme::AccentGreen);
        drawKPICard(625.0f, 18.0f, 220.0f, 68.0f, "DOAN TAU DANG QUAN LY", to_string(trCount) + " Chuyen (P1-P3)", GuiTheme::AccentCyan);
        drawKPICard(860.0f, 18.0f, 220.0f, 68.0f, "LICH TRINH DANG QUAN LY", to_string(trCount) + " Doan tau", GuiTheme::AccentGold);
        drawKPICard(1095.0f, 18.0f, 240.0f, 68.0f, "LOG GIAI TOA XUNG DOT", to_string(trainList.getLogCount()) + " Su kien gan nhat", GuiTheme::AccentPurple);

        for (int g = 0; g < NUM_GROUPS; ++g) {
            menuGroups[g].mainCategoryBtn.draw(window);
            if (menuGroups[g].isExpanded) {
                for (int s = 0; s < menuGroups[g].subCount; ++s) {
                    menuGroups[g].subButtons[s].draw(window);
                }
            }
        }

        sf::RectangleShape outputBox(sf::Vector2f(945.0f, 445.0f));
        outputBox.setPosition(sf::Vector2f(390.0f, 105.0f));
        outputBox.setFillColor(GuiTheme::PanelBg);
        outputBox.setOutlineThickness(1.5f);
        outputBox.setOutlineColor(GuiTheme::Border);
        window.draw(outputBox);

        sf::View defaultView = window.getView();
        sf::View scrollView(sf::FloatRect({390.0f, 105.0f}, {945.0f, 445.0f}));
        scrollView.setViewport(sf::FloatRect(
            {390.0f / 1360.0f, 105.0f / 768.0f},
            {945.0f / 1360.0f, 445.0f / 768.0f}
        ));
        window.setView(scrollView);

        // Render giao diện theo chế độ view được chọn
        if (activeViewMode == 1) {
            renderStationTable(window, font, stationList, 150.0f + scrollOffsetY, stationSearch);
        } else if (activeViewMode == 4) {
            renderTrainTable(window, font, trainList, 150.0f + scrollOffsetY, trainSearch);
        } else if (activeViewMode == 5) {
            renderTrainDetailsPanel(window, font, trainList, stationList, 150.0f + scrollOffsetY,
                                    stopStationSearch);
        } else {
            sf::Text logText(font, outputLog, 13);
            logText.setFillColor(GuiTheme::TextLight);
            logText.setPosition(sf::Vector2f(410.0f, 120.0f + scrollOffsetY));
            window.draw(logText);
        }

        window.setView(defaultView);
        if (activeViewMode == 1 || activeViewMode == 4 || activeViewMode == 5) {
            const string filterLabel = activeViewMode == 1 ? "Tim ID ga: " :
                (activeViewMode == 4 ? "Tim ma tau: " : "Tim ID ga dung: ");
            const string& filter = activeViewMode == 1 ? stationSearch :
                (activeViewMode == 4 ? trainSearch : stopStationSearch);
            sf::RectangleShape searchBox({265.0f, 28.0f});
            searchBox.setPosition({1055.0f, 112.0f});
            searchBox.setFillColor(GuiTheme::BgMain);
            searchBox.setOutlineThickness(1.5f);
            searchBox.setOutlineColor(searchFocus != 0 ? GuiTheme::AccentCyan : GuiTheme::Border);
            window.draw(searchBox);
            sf::Text searchText(font, filterLabel + filter, 12);
            searchText.setFillColor(GuiTheme::TextLight);
            searchText.setPosition({1063.0f, 118.0f});
            window.draw(searchText);
        }

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
            int startIdx = std::max(0, trainList.getLogCount() - 6);
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