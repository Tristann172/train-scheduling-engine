#include "GraphicalSimulationEngine.h"
#include "StationList.h"
#include "TrainList.h"
#include "StationNode.h"
#include "TrainNode.h"
#include "StopScheduleNode.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <limits>

using namespace std;

// ==================== 1. BẢNG MÀU HIỆN ĐẠI (DRACULA & CONTROL ROOM DARK) ====================
namespace Theme {
    const sf::Color BgCanvas(18, 20, 29);           // #12141D - Nền chính tối sâu
    const sf::Color BgGridLine(26, 29, 42);         // #1A1D2A - Đường lưới tọa độ mờ
    const sf::Color HudPanelBg(28, 32, 46, 230);    // #1C202E - Nền khung HUD bán trong suốt
    const sf::Color HudBorder(62, 70, 96);          // #3E4660 - Viền khung HUD
    const sf::Color BallastBed(38, 42, 58);         // #262A3A - Nền đá dăm đường sắt
    const sf::Color SleeperWood(65, 72, 98);        // #414862 - Thanh tà vẹt
    const sf::Color MainRailSteel(165, 178, 198);   // #A5B2C6 - Thanh ray thép chính
    const sf::Color SidingRail(98, 114, 164);       // #6272A4 - Thanh ray phụ

    const sf::Color StationPlatform(45, 52, 74);    // #2D344A - Sân ga
    const sf::Color StationCore(80, 250, 123);      // #50FA7B - Tâm nhà ga
    const sf::Color SignalGreen(46, 213, 115);      // #2ED573 - Tín hiệu Xanh an toàn
    const sf::Color SignalRed(255, 71, 87);         // #FF4757 - Tín hiệu Đỏ cảnh báo

    const sf::Color TrainRunning(0, 229, 255);      // #00E5FF - Tàu đang chạy khu gian (Cyan)
    const sf::Color TrainMainStop(255, 190, 11);    // #FFBE0B - Tàu đỗ ray chính (Amber)
    const sf::Color TrainSidingWait(189, 147, 249); // #BD93F9 - Tàu tránh ray phụ (Purple)
    const sf::Color TrainShadow(0, 0, 0, 140);      // Đổ bóng đoàn tàu

    const sf::Color TextPrimary(248, 248, 242);     // #F8F8F2 - Chữ trắng sáng
    const sf::Color TextSecondary(148, 163, 184);   // #94A3B8 - Chữ xám phụ trợ
    const sf::Color AccentGold(241, 250, 140);      // #F1FA8C - Nhấn vàng sáng
}

// Cấu trúc lưu trạng thái tọa độ Y của từng tàu để nội suy Lerp (Không dùng std::map)
struct TrainSmoothState {
    string code;
    float currentY;
};

// ==================== 2. CÁC HÀM ĐỒ HỌA PHỤ TRỢ (SHAPES & VERTEX ARRAY) ====================

// Tạo hình chữ nhật bo góc bằng sf::ConvexShape
static sf::ConvexShape createRoundedRect(float width, float height, float radius, unsigned int cornerPoints = 6) {
    radius = std::min(radius, std::min(width, height) / 2.0f);
    sf::ConvexShape shape;
    shape.setPointCount(cornerPoints * 4);

    const float PI = 3.14159265359f;
    unsigned int idx = 0;

    sf::Vector2f centers[4] = {
        {width - radius, height - radius},
        {radius, height - radius},
        {radius, radius},
        {width - radius, radius}
    };

    for (int c = 0; c < 4; ++c) {
        float startAngle = c * (PI / 2.0f);
        for (unsigned int i = 0; i < cornerPoints; ++i) {
            float angle = startAngle + (static_cast<float>(i) / (cornerPoints - 1)) * (PI / 2.0f);
            shape.setPoint(idx++, {
                centers[c].x + radius * std::cos(angle),
                centers[c].y + radius * std::sin(angle)
            });
        }
    }
    shape.setOrigin({width / 2.0f, height / 2.0f});
    return shape;
}

// Thêm một hình chữ nhật (ghép từ 2 tam giác) vào VertexArray trong SFML 3.0
static void appendQuadAsTriangles(sf::VertexArray& va, float x, float y, float w, float h, const sf::Color& color) {
    sf::Vector2f p0(x, y), p1(x + w, y), p2(x + w, y + h), p3(x, y + h);
    va.append(sf::Vertex{p0, color});
    va.append(sf::Vertex{p1, color});
    va.append(sf::Vertex{p2, color});
    va.append(sf::Vertex{p0, color});
    va.append(sf::Vertex{p2, color});
    va.append(sf::Vertex{p3, color});
}

// Vẽ đoạn thẳng có độ dày tùy chỉnh (dùng cho ghi rẽ nhánh đường ray phụ)
static void drawThickLine(sf::RenderWindow& window, sf::Vector2f p1, sf::Vector2f p2, float thickness, sf::Color color) {
    sf::Vector2f dir = p2 - p1;
    float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (length <= 0.001f) return;

    sf::RectangleShape line({length, thickness});
    line.setOrigin({0.0f, thickness / 2.0f});
    line.setPosition(p1);
    float angleRad = std::atan2(dir.y, dir.x);
    line.setRotation(sf::radians(angleRad));
    line.setFillColor(color);
    window.draw(line);
}

// ==================== 3. HÀM TÍNH TỌA ĐỘ KM MƯỢT THEO THỜI GIAN THỰC ====================
static double calculateTrainKmSmooth(TrainNode* train, StationList* stationList, float currentMinute, int& trackUsedOut) {
    trackUsedOut = 0;
    LinkedList<StopScheduleNode*>::Node* firstStopNode = train->getHeadSchedule();
    StopScheduleNode* firstStop = firstStopNode ? firstStopNode->data : nullptr;
    if (!firstStop) return -1.0;

    LinkedList<StopScheduleNode*>::Node* lastStopNode = train->getTailSchedule();
    StopScheduleNode* lastStop = lastStopNode ? lastStopNode->data : nullptr;

    // 1. Chưa tới giờ xuất phát -> Tàu chờ sẵn tại Ga Đầu
    if (currentMinute < static_cast<float>(firstStop->getArrivalMinute())) {
        StationNode* st = stationList->findStationByID(firstStop->getStationID());
        if (st) {
            trackUsedOut = firstStop->getTrackUsed();
            return st->getKmMarker();
        }
    }

    // 2. Đã qua giờ về ga cuối -> Tàu đỗ tại Ga Cuối
    if (currentMinute > static_cast<float>(lastStop->getDepartureMinute())) {
        StationNode* st = stationList->findStationByID(lastStop->getStationID());
        if (st) {
            trackUsedOut = lastStop->getTrackUsed();
            return st->getKmMarker();
        }
    }

    // 3. Kiểm tra tàu đang đỗ tại ga hoặc đang di chuyển giữa 2 ga (Khu gian)
    for (LinkedList<StopScheduleNode*>::Node* stopNode = firstStopNode;
         stopNode != nullptr; stopNode = stopNode->next) {
        StopScheduleNode* curr = stopNode->data;
        float arr = static_cast<float>(curr->getArrivalMinute());
        float dep = static_cast<float>(curr->getDepartureMinute());

        if (currentMinute >= arr && currentMinute <= dep) {
            StationNode* st = stationList->findStationByID(curr->getStationID());
            if (st) {
                trackUsedOut = curr->getTrackUsed();
                return st->getKmMarker();
            }
        }

        StopScheduleNode* nextStop =
            stopNode->next ? stopNode->next->data : nullptr;
        if (nextStop != nullptr) {
            float nextArr = static_cast<float>(nextStop->getArrivalMinute());
            if (currentMinute > dep && currentMinute < nextArr) {
                StationNode* st1 = stationList->findStationByID(curr->getStationID());
                StationNode* st2 = stationList->findStationByID(nextStop->getStationID());
                if (st1 && st2 && nextArr > dep) {
                    trackUsedOut = 0; // 0: Đang chạy trên khu gian đơn
                    double ratio = static_cast<double>(currentMinute - dep) / static_cast<double>(nextArr - dep);
                    return st1->getKmMarker() + ratio * (st2->getKmMarker() - st1->getKmMarker());
                }
            }
        }
    }

    return -1.0;
}

// ==================== 4. VÒNG LẶP ĐỒ HỌA CHÍNH (GRAPHICAL SIMULATION ENGINE) ====================
void GraphicalSimulationEngine::runSFMLSimulation(StationList* stationList, TrainList* trainList) {
    if (!stationList || !trainList) return;

    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}),
                            "PBL2: MO PHONG DIEU PHOI DUONG RAY & CHAY TAU (SFML GRAPHICS)",
                            sf::Style::Default, sf::State::Windowed, settings);
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("arial.ttf")) {
        cout << "[Canh bao] Khong tim thay font 'arial.ttf'!\n";
    }

    // Tìm mốc Km lớn nhất trên tuyến để chuẩn hóa tỷ lệ vẽ
    double maxKm = 100.0;
    for (LinkedList<StationNode*>::Node* station = stationList->getHead();
         station != nullptr; station = station->next) {
        StationNode* st = station->data;
        if (st->getKmMarker() > maxKm) maxKm = st->getKmMarker();
    }
    if (maxKm <= 0.0) maxKm = 100.0;

    const float marginX = 95.0f;
    const float trackY = 285.0f;
    const float trackWidth = WINDOW_WIDTH - 2.0f * marginX;
    const float sidingSpacingY = 44.0f;

    // Khởi tạo VertexArray cho Đường ray chính & Tà vẹt (Chỉ tốn 1 Draw Call mỗi khung hình)
    sf::VertexArray mainTrackVA(sf::PrimitiveType::Triangles);
    appendQuadAsTriangles(mainTrackVA, marginX - 20.0f, trackY - 8.0f, trackWidth + 40.0f, 16.0f, Theme::BallastBed);
    for (float x = marginX - 10.0f; x <= marginX + trackWidth + 10.0f; x += 12.0f) {
        appendQuadAsTriangles(mainTrackVA, x, trackY - 7.0f, 4.0f, 14.0f, Theme::SleeperWood);
    }
    appendQuadAsTriangles(mainTrackVA, marginX - 20.0f, trackY - 3.5f, trackWidth + 40.0f, 2.2f, Theme::MainRailSteel);
    appendQuadAsTriangles(mainTrackVA, marginX - 20.0f, trackY + 1.5f, trackWidth + 40.0f, 2.2f, Theme::MainRailSteel);

    // Mảng tĩnh quản lý hiệu ứng chuyển làn mượt (Smooth Y-Lerp) không dùng STL
    static const int MAX_SMOOTH_TRAINS = 64;
    TrainSmoothState smoothStates[MAX_SMOOTH_TRAINS];
    int smoothCount = 0;

    auto getSmoothY = [&](const string& code, float targetY, float dt) -> float {
        for (int i = 0; i < smoothCount; ++i) {
            if (smoothStates[i].code == code) {
                float lerpFactor = std::min(1.0f, dt * 8.5f);
                smoothStates[i].currentY += (targetY - smoothStates[i].currentY) * lerpFactor;
                return smoothStates[i].currentY;
            }
        }
        if (smoothCount < MAX_SMOOTH_TRAINS) {
            smoothStates[smoothCount].code = code;
            smoothStates[smoothCount].currentY = targetY;
            smoothCount++;
        }
        return targetY;
    };

    // Tự động lấy mốc thời gian bắt đầu sớm nhất của các đoàn tàu (trừ hao 15 phút chuẩn bị)
    int earliestMinute = std::numeric_limits<int>::max();
    int latestMinute = 0;
    for (LinkedList<TrainNode*>::Node* trainNode = trainList->getHead();
         trainNode != nullptr; trainNode = trainNode->next) {
        TrainNode* tr = trainNode->data;
        for (LinkedList<StopScheduleNode*>::Node* stop = tr->getHeadSchedule();
             stop; stop = stop->next) {
            earliestMinute = std::min(earliestMinute, stop->data->getArrivalMinute());
            latestMinute = std::max(latestMinute, stop->data->getDepartureMinute());
        }
    }
    if (earliestMinute == std::numeric_limits<int>::max()) earliestMinute = 0;
    const float endMinute = static_cast<float>(std::max(latestMinute, earliestMinute) + 60);
    float startMinute = static_cast<float>(std::max(0, earliestMinute - 15));
    float currentMinute = startMinute;

    float timeSpeed = 30.0f;
    bool isPaused = false;
    sf::Clock deltaClock;
    sf::Clock animClock;

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Space) {
                    isPaused = !isPaused;
                } else if (keyPressed->code == sf::Keyboard::Key::Up) {
                    timeSpeed = std::min(240.0f, timeSpeed + 15.0f);
                } else if (keyPressed->code == sf::Keyboard::Key::Down) {
                    timeSpeed = std::max(5.0f, timeSpeed - 15.0f);
                } else if (keyPressed->code == sf::Keyboard::Key::Right) {
                    currentMinute = std::min(endMinute, currentMinute + 15.0f);
                } else if (keyPressed->code == sf::Keyboard::Key::Left) {
                    currentMinute = std::max(0.0f, currentMinute - 15.0f);
                } else if (keyPressed->code == sf::Keyboard::Key::R) {
                    currentMinute = startMinute;
                } else if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
            }
        }

        float dt = deltaClock.restart().asSeconds();
        float animTime = animClock.getElapsedTime().asSeconds();

        if (!isPaused) {
            currentMinute = std::min(endMinute, currentMinute + timeSpeed * dt);
        }

        window.clear(Theme::BgCanvas);

        // ==================== A. VẼ LƯỚI TỌA ĐỘ NỀN (SUBTLE GRID) ====================
        for (float gx = marginX; gx <= marginX + trackWidth; gx += 120.0f) {
            sf::RectangleShape vLine({1.0f, 390.0f});
            vLine.setPosition({gx, 105.0f});
            vLine.setFillColor(Theme::BgGridLine);
            window.draw(vLine);
        }

        // ==================== B. VẼ NHÀ GA, ĐƯỜNG RAY PHỤ & ĐÈN TÍN HIỆU ====================
        for (LinkedList<StationNode*>::Node* station = stationList->getHead();
             station != nullptr; station = station->next) {
            StationNode* st = station->data;
            float posX = marginX + static_cast<float>(st->getKmMarker() / maxKm) * trackWidth;
            int maxTracks = st->getMaxTracks();
            const int simulationMinute = static_cast<int>(currentMinute);
            const bool stationClosed = trainList->isUnderMaintenanceAt(
                "STATION", st->getStationID(), 0, simulationMinute);

            // Kiểm tra có đoàn tàu nào đang dừng đỗ tại ga này không để bật đèn tín hiệu Đỏ
            bool stationOccupied = false;
            for (LinkedList<TrainNode*>::Node* trainNode = trainList->getHead();
                 trainNode != nullptr; trainNode = trainNode->next) {
                TrainNode* tr = trainNode->data;
                for (LinkedList<StopScheduleNode*>::Node* stopNode =
                         tr->getHeadSchedule(); stopNode != nullptr;
                     stopNode = stopNode->next) {
                    StopScheduleNode* sc = stopNode->data;
                    if (sc->getStationID() == st->getStationID()) {
                        if (currentMinute >= static_cast<float>(sc->getArrivalMinute()) &&
                            (currentMinute <= static_cast<float>(sc->getDepartureMinute()) ||
                             (stopNode->next == nullptr &&
                              currentMinute <= endMinute))) {
                            stationOccupied = true;
                            break;
                        }
                    }
                }
                if (stationOccupied) break;
            }

            // 1. Trục gióng dọc tại tâm nhà ga
            float totalStationHeight = (maxTracks - 1) * sidingSpacingY + 75.0f;
            sf::RectangleShape guideLine({2.0f, totalStationHeight});
            guideLine.setPosition({posX - 1.0f, trackY - 35.0f});
            guideLine.setFillColor(stationClosed ? Theme::SignalRed : sf::Color(62, 70, 96, 110));
            window.draw(guideLine);

            // 2. Các đường ray phụ (Siding Tracks: từ Ray 2 đến maxTracks)
            for (int t = 2; t <= maxTracks; ++t) {
                float sidY = trackY + (t - 1) * sidingSpacingY;
                float prevY = trackY + (t - 2) * sidingSpacingY;
                float halfLen = 46.0f;
                const bool sidingClosed = stationClosed || trainList->isUnderMaintenanceAt(
                    "TRACK", st->getStationID(), t, simulationMinute);
                const sf::Color sidingColor = sidingClosed ? Theme::SignalRed : Theme::SidingRail;

                // Ghi rẽ nhánh trái & phải nối chéo lên ray phía trên
                drawThickLine(window, {posX - halfLen - 18.0f, prevY}, {posX - halfLen, sidY}, 3.0f, sidingColor);
                drawThickLine(window, {posX + halfLen, sidY}, {posX + halfLen + 18.0f, prevY}, 3.0f, sidingColor);

                // Tà vẹt trên ray phụ
                for (float sx = posX - halfLen; sx <= posX + halfLen; sx += 10.0f) {
                    sf::RectangleShape slp({3.0f, 10.0f});
                    slp.setOrigin({1.5f, 5.0f});
                    slp.setPosition({sx, sidY});
                    slp.setFillColor(sidingClosed ? Theme::SignalRed : Theme::SleeperWood);
                    window.draw(slp);
                }

                // Thanh ray phụ
                sf::RectangleShape sidingBar({halfLen * 2.0f, 3.5f});
                sidingBar.setOrigin({halfLen, 1.75f});
                sidingBar.setPosition({posX, sidY});
                sidingBar.setFillColor(sidingColor);
                window.draw(sidingBar);

                // Nhãn ký hiệu ray phụ (R2, R3...)
                sf::Text trkLabel(font, "R" + to_string(t), 12);
                trkLabel.setFillColor(sidingClosed ? Theme::SignalRed : Theme::TextSecondary);
                trkLabel.setPosition({posX - halfLen - 18.0f, sidY - 6.0f});
                window.draw(trkLabel);
            }

            // 3. Vùng Sân ga (Station Platform Badge)
            sf::ConvexShape platform = createRoundedRect(92.0f, 24.0f, 6.0f);
            platform.setPosition({posX, trackY - 58.0f});
            platform.setFillColor(stationClosed ? sf::Color(100, 28, 38) : Theme::StationPlatform);
            platform.setOutlineThickness(1.5f);
            platform.setOutlineColor(stationClosed ? Theme::SignalRed : Theme::HudBorder);
            window.draw(platform);

            sf::Text stTitle(font, st->getName(), 14);
            stTitle.setFillColor(Theme::AccentGold);
            sf::FloatRect tb = stTitle.getLocalBounds();
            stTitle.setPosition({posX - tb.size.x / 2.0f, trackY - 66.0f});
            window.draw(stTitle);

            sf::Text stSub(font, "ID:" + to_string(st->getStationID()) + " | " +
                           to_string(static_cast<int>(st->getKmMarker())) + "km (" + to_string(maxTracks) + "R)", 12);
            stSub.setFillColor(stationClosed ? Theme::SignalRed : Theme::TextSecondary);
            sf::FloatRect sb = stSub.getLocalBounds();
            stSub.setPosition({posX - sb.size.x / 2.0f, trackY - 42.0f});
            window.draw(stSub);

            // 4. Đèn tín hiệu nhà ga (Xanh / Đỏ nhấp nháy)
            sf::Color sigColor = stationClosed || stationOccupied
                ? Theme::SignalRed : Theme::SignalGreen;
            float pulse = stationClosed || stationOccupied
                ? (0.55f + 0.45f * std::sin(animTime * 7.0f)) : 1.0f;

            sf::CircleShape sigGlow(10.0f);
            sigGlow.setOrigin({10.0f, 10.0f});
            sigGlow.setPosition({posX + 38.0f, trackY - 22.0f});
            sigGlow.setFillColor(sf::Color(sigColor.r, sigColor.g, sigColor.b, static_cast<uint8_t>(90 * pulse)));
            window.draw(sigGlow);

            sf::CircleShape sigLamp(4.5f);
            sigLamp.setOrigin({4.5f, 4.5f});
            sigLamp.setPosition({posX + 38.0f, trackY - 22.0f});
            sigLamp.setFillColor(sf::Color(sigColor.r, sigColor.g, sigColor.b, static_cast<uint8_t>(255 * pulse)));
            sigLamp.setOutlineThickness(1.5f);
            sigLamp.setOutlineColor(sf::Color(20, 20, 25));
            window.draw(sigLamp);
        }

        // ==================== C. VẼ ĐƯỜNG RAY CHÍNH & CÁC TÂM GA ====================
        window.draw(mainTrackVA);

        for (LinkedList<StationNode*>::Node* station = stationList->getHead();
             station != nullptr; station = station->next) {
            StationNode* first = station->data;
            const int minute = static_cast<int>(currentMinute);
            const bool stationClosed = trainList->isUnderMaintenanceAt(
                "STATION", first->getStationID(), 0, minute);
            const bool mainTrackClosed = stationClosed ||
                trainList->isUnderMaintenanceAt("TRACK", first->getStationID(), 1, minute);
            const float firstX = marginX +
                static_cast<float>(first->getKmMarker() / maxKm) * trackWidth;
            if (mainTrackClosed) {
                drawThickLine(window, {firstX - 46.0f, trackY},
                              {firstX + 46.0f, trackY}, 5.0f, Theme::SignalRed);
            }
            for (LinkedList<StationNode*>::Node* other = stationList->getHead();
                 other != nullptr; other = other->next) {
                StationNode* second = other->data;
                if (first->getStationID() < second->getStationID() &&
                    trainList->isUnderMaintenanceAt("BLOCK", first->getStationID(),
                        second->getStationID(), minute)) {
                    const float secondX = marginX +
                        static_cast<float>(second->getKmMarker() / maxKm) * trackWidth;
                    drawThickLine(window, {firstX, trackY}, {secondX, trackY},
                                  5.0f, Theme::SignalRed);
                }
            }
        }

        for (LinkedList<StationNode*>::Node* station = stationList->getHead();
             station != nullptr; station = station->next) {
            StationNode* st = station->data;
            float posX = marginX + static_cast<float>(st->getKmMarker() / maxKm) * trackWidth;
            const bool stationClosed = trainList->isUnderMaintenanceAt(
                "STATION", st->getStationID(), 0, static_cast<int>(currentMinute));

            sf::CircleShape outerNode(9.0f);
            outerNode.setOrigin({9.0f, 9.0f});
            outerNode.setPosition({posX, trackY});
            outerNode.setFillColor(Theme::BgCanvas);
            outerNode.setOutlineThickness(2.5f);
            outerNode.setOutlineColor(stationClosed ? Theme::SignalRed : Theme::StationCore);
            window.draw(outerNode);

            sf::CircleShape innerNode(4.0f);
            innerNode.setOrigin({4.0f, 4.0f});
            innerNode.setPosition({posX, trackY});
            innerNode.setFillColor(stationClosed ? Theme::SignalRed : Theme::StationCore);
            window.draw(innerNode);
        }

        // ==================== D. VẼ CÁC ĐOÀN TÀU BO GÓC, ĐỔ BÓNG & NỘI SUY LERP ====================
        for (LinkedList<TrainNode*>::Node* trainNode = trainList->getHead();
             trainNode != nullptr; trainNode = trainNode->next) {
            TrainNode* train = trainNode->data;
            int trackUsed = 0;
            double currentKm = calculateTrainKmSmooth(train, stationList, currentMinute, trackUsed);

            if (currentKm >= 0.0) {
                float posX = marginX + static_cast<float>(currentKm / maxKm) * trackWidth;
                float targetY = trackY + (trackUsed > 1 ? (trackUsed - 1) * sidingSpacingY : 0.0f);
                float posY = getSmoothY(train->getTrainCode(), targetY, dt);

                sf::Color bodyColor = Theme::TrainRunning;
                if (trackUsed == 1) bodyColor = Theme::TrainMainStop;
                else if (trackUsed > 1) bodyColor = Theme::TrainSidingWait;

                // 1. Chùm đèn pha chiếu sáng hướng chạy khi tàu di chuyển trên khu gian
                if (trackUsed == 0) {
                    float dirSign = (train->getDirection() == 1) ? 1.0f : -1.0f;
                    sf::ConvexShape beam;
                    beam.setPointCount(3);
                    beam.setPoint(0, {posX + dirSign * 26.0f, posY});
                    beam.setPoint(1, {posX + dirSign * 72.0f, posY - 12.0f});
                    beam.setPoint(2, {posX + dirSign * 72.0f, posY + 12.0f});
                    beam.setFillColor(sf::Color(255, 250, 190, 55));
                    window.draw(beam);
                }

                // 2. Lớp đổ bóng (Drop Shadow)
                sf::ConvexShape shadowShape = createRoundedRect(54.0f, 22.0f, 7.0f);
                shadowShape.setPosition({posX + 3.0f, posY + 4.5f});
                shadowShape.setFillColor(Theme::TrainShadow);
                window.draw(shadowShape);

                // 3. Thân tàu bo góc (Rounded Train Body)
                sf::ConvexShape trainShape = createRoundedRect(54.0f, 22.0f, 7.0f);
                trainShape.setPosition({posX, posY});
                trainShape.setFillColor(bodyColor);
                trainShape.setOutlineThickness(1.5f);
                trainShape.setOutlineColor(sf::Color(255, 255, 255, 210));
                window.draw(trainShape);

                // 4. Dải cửa sổ kính trên thân tàu
                sf::ConvexShape windowStrip = createRoundedRect(36.0f, 6.0f, 2.5f);
                windowStrip.setPosition({posX, posY - 3.5f});
                windowStrip.setFillColor(sf::Color(18, 22, 34, 200));
                window.draw(windowStrip);

                // 5. Nhãn Mã tàu + Cấp ưu tiên (P1, P2, P3)
                string code = train->getTrainCode();
                string badgeStr = code + " [P" + to_string(train->getPriority()) + "]";
                sf::Text trainLabel(font, badgeStr, 12);
                trainLabel.setFillColor(sf::Color(15, 18, 25));
                sf::FloatRect lb = trainLabel.getLocalBounds();
                trainLabel.setPosition({posX - lb.size.x / 2.0f, posY - 0.5f});
                window.draw(trainLabel);

                // Nhãn hướng chạy nổi phía trên đoàn tàu
                sf::Text dirTag(font, (train->getDirection() == 1 ? code + " ->" : "<- " + code), 13);
                dirTag.setFillColor(Theme::TextPrimary);
                sf::FloatRect db = dirTag.getLocalBounds();
                dirTag.setPosition({posX - db.size.x / 2.0f, posY - 26.0f});
                window.draw(dirTag);
            }
        }

        // ==================== E. BẢNG ĐIỀU KHIỂN HUD (HEADS-UP DISPLAY OVERLAY) ====================
        // 1. Thanh điều khiển Top Bar HUD
        sf::ConvexShape topHud = createRoundedRect(WINDOW_WIDTH - 40.0f, 58.0f, 10.0f);
        topHud.setPosition({WINDOW_WIDTH / 2.0f, 38.0f});
        topHud.setFillColor(Theme::HudPanelBg);
        topHud.setOutlineThickness(1.5f);
        topHud.setOutlineColor(Theme::HudBorder);
        window.draw(topHud);

        const int absoluteMinute = static_cast<int>(currentMinute);
        const int day = absoluteMinute / 1440;
        int hours = (absoluteMinute / 60) % 24;
        int mins  = static_cast<int>(currentMinute) % 60;
        int secs  = static_cast<int>((currentMinute - std::floor(currentMinute)) * 60.0f);

        stringstream clockSS;
        if (day > 0) clockSS << "D+" << day << " ";
        clockSS << setfill('0') << setw(2) << hours << ":"
                << setfill('0') << setw(2) << mins << ":"
                << setfill('0') << setw(2) << secs;

        sf::Text clockText(font, "TIME: " + clockSS.str(), 20);
        clockText.setFillColor(Theme::AccentGold);
        clockText.setPosition({45.0f, 24.0f});
        window.draw(clockText);

        stringstream ctrlSS;
        ctrlSS << (isPaused ? "[TAM DUNG]" : "[DANG CHAY]")
               << "  |  TOC DO: " << static_cast<int>(timeSpeed) << "x"
               << "  |  [SPACE] Pause  [UP/DOWN] Toc do  [LEFT/RIGHT] Tua +/-15p  [R] Reset  [ESC] Thoat";
        sf::Text ctrlText(font, ctrlSS.str(), 13);
        ctrlText.setFillColor(Theme::TextPrimary);
        ctrlText.setPosition({255.0f, 28.0f});
        window.draw(ctrlText);

        // Chú giải màu sắc (Legend)
        auto drawLegendItem = [&](float lx, float ly, sf::Color col, const string& textStr) {
            sf::CircleShape dot(6.0f);
            dot.setPosition({lx, ly + 2.0f});
            dot.setFillColor(col);
            window.draw(dot);

            sf::Text lbl(font, textStr, 12);
            lbl.setFillColor(Theme::TextSecondary);
            lbl.setPosition({lx + 18.0f, ly});
            window.draw(lbl);
        };
        drawLegendItem(45.0f, 76.0f, Theme::TrainRunning, "Chay khu gian (Cyan)");
        drawLegendItem(215.0f, 76.0f, Theme::TrainMainStop, "Do Ray chinh R1 (Amber)");
        drawLegendItem(400.0f, 76.0f, Theme::TrainSidingWait, "Tranh tau Ray phu R2..N (Purple)");
        drawLegendItem(635.0f, 76.0f, Theme::SignalRed, "Bao tri/ga co tau chiem");
        drawLegendItem(845.0f, 76.0f, Theme::SignalGreen, "Den Xanh: Ga thong thoang");

        // 2. Khung Nhật ký Giải quyết Xung đột (Đọc trực tiếp từ Circular Buffer của TrainList)
        float logW = 760.0f, logH = 220.0f;
        sf::ConvexShape logPanel = createRoundedRect(logW, logH, 10.0f);
        logPanel.setPosition({20.0f + logW / 2.0f, WINDOW_HEIGHT - 20.0f - logH / 2.0f});
        logPanel.setFillColor(Theme::HudPanelBg);
        logPanel.setOutlineThickness(1.5f);
        logPanel.setOutlineColor(Theme::HudBorder);
        window.draw(logPanel);

        sf::Text logTitle(font, "NHAT KY DIEU PHOI & GIAI QUYET XUNG DOT (MEET & PASS - CIRCULAR BUFFER LOGS)", 13);
        logTitle.setFillColor(Theme::AccentGold);
        logTitle.setPosition({38.0f, WINDOW_HEIGHT - logH - 8.0f});
        window.draw(logTitle);

        int totalLogs = trainList->getLogCount();
        if (totalLogs == 0) {
            sf::Text emptyLog(font, "-> He thong dang van hanh on dinh. Chua phat sinh xung dot moi hoac da giai toa xong.", 12);
            emptyLog.setFillColor(Theme::TextSecondary);
            emptyLog.setPosition({42.0f, WINDOW_HEIGHT - logH + 22.0f});
            window.draw(emptyLog);
        } else {
            float ly = WINDOW_HEIGHT - logH + 18.0f;
            for (int i = 0; i < totalLogs; ++i) {
                sf::Text logLine(font, ">> " + trainList->getLog(i), 12);
                logLine.setFillColor(Theme::TextPrimary);
                logLine.setPosition({42.0f, ly});
                window.draw(logLine);
                ly += 21.0f;
            }
        }

        // 3. Bảng Giám sát Trạng thái Đoàn tàu Thời gian thực (Bottom-Right Telemetry Panel)
        float teleW = 535.0f, teleH = 220.0f;
        float teleCenterX = WINDOW_WIDTH - 20.0f - teleW / 2.0f;
        sf::ConvexShape telePanel = createRoundedRect(teleW, teleH, 10.0f);
        telePanel.setPosition({teleCenterX, WINDOW_HEIGHT - 20.0f - teleH / 2.0f});
        telePanel.setFillColor(Theme::HudPanelBg);
        telePanel.setOutlineThickness(1.5f);
        telePanel.setOutlineColor(Theme::HudBorder);
        window.draw(telePanel);

        float teleLeft = WINDOW_WIDTH - 20.0f - teleW + 18.0f;
        sf::Text teleTitle(font, "BANG GIAM SAT TRANG THAI DOAN TAU (LIVE TELEMETRY)", 13);
        teleTitle.setFillColor(Theme::StationCore);
        teleTitle.setPosition({teleLeft, WINDOW_HEIGHT - teleH - 8.0f});
        window.draw(teleTitle);

        float ty = WINDOW_HEIGHT - teleH + 18.0f;
        for (LinkedList<TrainNode*>::Node* trainNode = trainList->getHead();
             trainNode != nullptr; trainNode = trainNode->next) {
            TrainNode* tr = trainNode->data;
            int trk = 0;
            double km = calculateTrainKmSmooth(tr, stationList, currentMinute, trk);
            string statusStr = (trk == 0) ? "Chay khu gian" : (trk == 1 ? "Do Ray Chinh (R1)" : "Tranh Ray Phu (R" + to_string(trk) + ")");

            stringstream rowSS;
            rowSS << left << setw(6) << tr->getTrainCode()
                  << " | Uu tien P" << tr->getPriority()
                  << " | " << (tr->getDirection() == 1 ? "Nam" : "Bac")
                  << " | Km " << setw(5) << static_cast<int>(std::max(0.0, km))
                  << " | " << statusStr;

            sf::Text rowTxt(font, rowSS.str(), 12);
            rowTxt.setFillColor(trk > 1 ? Theme::TrainSidingWait : (trk == 1 ? Theme::TrainMainStop : Theme::TrainRunning));
            rowTxt.setPosition({teleLeft, ty});
            window.draw(rowTxt);
            ty += 22.0f;
        }

        window.display();
    }
}