#include "GraphicalSimulationEngine.h"
#include "StationList.h"
#include "TrainList.h"
#include "StationNode.h"
#include "TrainNode.h"
#include "StopScheduleNode.h"

#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;

// Hàm tính vị trí Km của tàu chuẩn xác cho mọi mốc thời gian
static double calculateTrainKm(TrainNode* train, StationList* stationList, int currentMinute, int& trackUsedOut) {
    trackUsedOut = 0;
    StopScheduleNode* currStop = train->getHeadSchedule();
    if (!currStop) return -1.0;

    // Lấy thông tin ga đầu tiên và ga cuối cùng trong lịch trình
    StopScheduleNode* firstStop = currStop;
    StopScheduleNode* lastStop = currStop;
    while (lastStop->getNextStop() != nullptr) {
        lastStop = lastStop->getNextStop();
    }

    // 1. Trường hợp thời gian chưa đến giờ tàu xuất phát -> Cho tàu chờ sẵn tại Ga Đầu
    if (currentMinute < firstStop->getArrivalMinute()) {
        StationNode* st = stationList->findStationByID(firstStop->getStationID());
        if (st) {
            trackUsedOut = firstStop->getTrackUsed();
            return st->getKmMarker();
        }
    }

    // 2. Trường hợp thời gian đã qua giờ tàu về ga cuối -> Cho tàu đỗ tại Ga Cuối
    if (currentMinute > lastStop->getDepartureMinute()) {
        StationNode* st = stationList->findStationByID(lastStop->getStationID());
        if (st) {
            trackUsedOut = lastStop->getTrackUsed();
            return st->getKmMarker();
        }
    }

    // 3. Kiểm tra nếu tàu đang đỗ tại một ga trung gian
    currStop = train->getHeadSchedule();
    while (currStop != nullptr) {
        if (currentMinute >= currStop->getArrivalMinute() && currentMinute <= currStop->getDepartureMinute()) {
            StationNode* st = stationList->findStationByID(currStop->getStationID());
            if (st) {
                trackUsedOut = currStop->getTrackUsed();
                return st->getKmMarker();
            }
        }
        currStop = currStop->getNextStop();
    }

    // 4. Trường hợp tàu đang di chuyển giữa 2 ga (Khu gian)
    currStop = train->getHeadSchedule();
    while (currStop != nullptr && currStop->getNextStop() != nullptr) {
        StopScheduleNode* nextStop = currStop->getNextStop();

        if (currentMinute > currStop->getDepartureMinute() && currentMinute < nextStop->getArrivalMinute()) {
            StationNode* st1 = stationList->findStationByID(currStop->getStationID());
            StationNode* st2 = stationList->findStationByID(nextStop->getStationID());

            if (st1 && st2) {
                double km1 = st1->getKmMarker();
                double km2 = st2->getKmMarker();
                int t1 = currStop->getDepartureMinute();
                int t2 = nextStop->getArrivalMinute();

                double ratio = (double)(currentMinute - t1) / (t2 - t1);
                return km1 + ratio * (km2 - km1);
            }
        }
        currStop = currStop->getNextStop();
    }

    return -1.0;
}

void GraphicalSimulationEngine::runSFMLSimulation(StationList* stationList, TrainList* trainList) {
    if (!stationList || !trainList) return;

    sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), 
                            "PBL2: MO PHONG DIEU PHOI DUONG RAY & CHAY TAU (SFML GRAPHICS)");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("arial.ttf")) {
        cout << "[Coi chung] Khong tim thay font 'arial.ttf'!\n";
    }

    double maxKm = 800.0;
    StationNode* stLast = stationList->getHead();
    while (stLast && stLast->getNextStation()) stLast = stLast->getNextStation();
    if (stLast) maxKm = stLast->getKmMarker();

    float marginX = 100.0f;
    float trackY = 300.0f;
    float trackWidth = WINDOW_WIDTH - 2 * marginX;

    float currentMinute = 0.0f;
    float timeSpeed = 30.0f;
    sf::Clock deltaClock;

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Space) {
                    timeSpeed = (timeSpeed == 0.0f) ? 30.0f : 0.0f;
                }
            }
        }

        float dt = deltaClock.restart().asSeconds();
        currentMinute += timeSpeed * dt;
        if (currentMinute > 1440.0f) currentMinute = 0.0f;

        window.clear(sf::Color(30, 30, 35));

        // 1. Đồng hồ thời gian thực
        int hours = (int)currentMinute / 60;
        int mins = (int)currentMinute % 60;
        stringstream ss;
        ss << "PBL2 MO PHONG DIEU PHOI TAU HOA | THOI GIAN: "
           << setfill('0') << setw(2) << hours << ":" << setfill('0') << setw(2) << mins
           << " | (SPACE: Tam dung/Chay tiep)";

        sf::Text titleText(font, ss.str(), 20);
        titleText.setFillColor(sf::Color::White);
        titleText.setPosition(sf::Vector2f(marginX, 20.0f));
        window.draw(titleText);

        // 2. Đường ray chính (Main Track)
        sf::RectangleShape mainTrack(sf::Vector2f(trackWidth, 6.0f));
        mainTrack.setFillColor(sf::Color(150, 150, 150));
        mainTrack.setPosition(sf::Vector2f(marginX, trackY));
        window.draw(mainTrack);

        // 3. Hiển thị Nhà ga & Đường ray phụ
        StationNode* st = stationList->getHead();
        while (st != nullptr) {
            float posX = marginX + (float)(st->getKmMarker() / maxKm) * trackWidth;

            sf::CircleShape stationCircle(10.0f);
            stationCircle.setFillColor(sf::Color(46, 204, 113));
            stationCircle.setOrigin(sf::Vector2f(10.0f, 10.0f));
            stationCircle.setPosition(sf::Vector2f(posX, trackY + 3.0f));
            window.draw(stationCircle);

            sf::Text stName(font, st->getName() + "\n(" + to_string((int)st->getKmMarker()) + " km)", 12);
            stName.setFillColor(sf::Color(241, 196, 15));
            stName.setPosition(sf::Vector2f(posX - 25.0f, trackY + 20.0f));
            window.draw(stName);

            for (int t = 2; t <= st->getNumberOfTracks(); ++t) {
                float offsetY = (t - 1) * 35.0f;
                sf::RectangleShape sidingTrack(sf::Vector2f(80.0f, 4.0f));
                sidingTrack.setFillColor(sf::Color(100, 100, 200));
                sidingTrack.setOrigin(sf::Vector2f(40.0f, 2.0f));
                sidingTrack.setPosition(sf::Vector2f(posX, trackY + 3.0f + offsetY));
                window.draw(sidingTrack);
            }

            st = st->getNextStation();
        }

        // 4. Vẽ các đoàn tàu chuyển động
        TrainNode* train = trainList->getHead();
        while (train != nullptr) {
            int trackUsed = 0;
            double currentKm = calculateTrainKm(train, stationList, (int)currentMinute, trackUsed);

            if (currentKm >= 0.0) {
                float posX = marginX + (float)(currentKm / maxKm) * trackWidth;
                float posY = trackY;

                if (trackUsed > 1) {
                    posY += (trackUsed - 1) * 35.0f;
                }

                sf::RectangleShape trainRect(sf::Vector2f(45.0f, 20.0f));
                trainRect.setOrigin(sf::Vector2f(22.5f, 10.0f));
                trainRect.setPosition(sf::Vector2f(posX, posY + 3.0f));

                if (trackUsed == 0) {
                    trainRect.setFillColor(sf::Color(231, 76, 60));  // Đỏ: Chạy khu gian
                } else if (trackUsed == 1) {
                    trainRect.setFillColor(sf::Color(241, 196, 15)); // Vàng: Ray chính
                } else {
                    trainRect.setFillColor(sf::Color(155, 89, 182)); // Tím: Ray phụ
                }
                window.draw(trainRect);

                sf::Text trainText(font, train->getTrainCode(), 12);
                trainText.setFillColor(sf::Color::White);
                trainText.setPosition(sf::Vector2f(posX - 14.0f, posY - 25.0f));
                window.draw(trainText);
            }

            train = train->getNextTrain();
        }

        window.display();
    }
}
