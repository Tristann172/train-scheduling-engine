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

// Hàm phụ trợ nội suy vị trí Km của tàu theo phút trong ngày
static double calculateTrainKm(TrainNode* train, StationList* stationList, int currentMinute, int& trackUsedOut) {
    trackUsedOut = 0; // 0: Đang chạy khu gian, >0: Đang đỗ tại ray số x
    StopScheduleNode* currStop = train->getHeadSchedule();
    if (!currStop) return -1.0;

    // 1. Kiểm tra nếu tàu đang đỗ tại ga
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

    // 2. Nếu tàu đang di chuyển giữa 2 ga (Khu gian)
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

    return -1.0; // Tàu chưa chạy hoặc đã kết thúc hành trình
}

void GraphicalSimulationEngine::runSFMLSimulation(StationList* stationList, TrainList* trainList) {
    if (!stationList || !trainList) return;

    // Khởi tạo cửa sổ SFML
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), 
                            "PBL2: MO PHONG DIEU PHOI DUONG RAY & CHAY TAU (SFML GRAPHICS)", 
                            sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
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
    float timeSpeed = 20.0f;
    sf::Clock deltaClock;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space) {
                timeSpeed = (timeSpeed == 0.0f) ? 20.0f : 0.0f;
            }
        }

        float dt = deltaClock.restart().asSeconds();
        currentMinute += timeSpeed * dt;
        if (currentMinute > 1440.0f) currentMinute = 0.0f;

        window.clear(sf::Color(30, 30, 35));

        // 1. Tiêu đề đồng hồ thời gian thực
        sf::Text titleText;
        titleText.setFont(font);
        titleText.setCharacterSize(20);
        titleText.setFillColor(sf::Color::White);
        titleText.setPosition(marginX, 20.0f);
        
        int hours = (int)currentMinute / 60;
        int mins = (int)currentMinute % 60;
        stringstream ss;
        ss << "PBL2 MO PHONG DIEU PHOI TAU HOA | THOI GIAN: "
           << setfill('0') << setw(2) << hours << ":" << setfill('0') << setw(2) << mins
           << " | (SPACE: Tam dung/Chay tiếp)";
        titleText.setString(ss.str());
        window.draw(titleText);

        // 2. Đường ray chính (Main Track)
        sf::RectangleShape mainTrack(sf::Vector2f(trackWidth, 6.0f));
        mainTrack.setFillColor(sf::Color(150, 150, 150));
        mainTrack.setPosition(marginX, trackY);
        window.draw(mainTrack);

        // 3. Các nhà ga & Đường ray phụ (Siding Tracks)
        StationNode* st = stationList->getHead();
        while (st != nullptr) {
            float posX = marginX + (float)(st->getKmMarker() / maxKm) * trackWidth;

            sf::CircleShape stationCircle(10.0f);
            stationCircle.setFillColor(sf::Color(46, 204, 113));
            stationCircle.setOrigin(10.0f, 10.0f);
            stationCircle.setPosition(posX, trackY + 3.0f);
            window.draw(stationCircle);

            sf::Text stName;
            stName.setFont(font);
            stName.setString(st->getName() + "\n(" + to_string((int)st->getKmMarker()) + " km)");
            stName.setCharacterSize(13);
            stName.setFillColor(sf::Color(241, 196, 15));
            stName.setPosition(posX - 25.0f, trackY + 20.0f);
            window.draw(stName);

            // Vẽ các ray phụ
            for (int t = 2; t <= st->getNumberOfTracks(); ++t) {
                float offsetY = (t - 1) * 35.0f;
                sf::RectangleShape sidingTrack(sf::Vector2f(80.0f, 4.0f));
                sidingTrack.setFillColor(sf::Color(100, 100, 200));
                sidingTrack.setOrigin(40.0f, 2.0f);
                sidingTrack.setPosition(posX, trackY + 3.0f + offsetY);
                window.draw(sidingTrack);
            }

            st = st->getNextStation();
        }

        // 4. Các đoàn tàu chuyển động
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

                sf::RectangleShape trainRect(sf::Vector2f(40.0f, 18.0f));
                trainRect.setOrigin(20.0f, 9.0f);
                trainRect.setPosition(posX, posY + 3.0f);

                if (trackUsed == 0) {
                    trainRect.setFillColor(sf::Color(231, 76, 60));  // Đỏ: Chạy khu gian
                } else if (trackUsed == 1) {
                    trainRect.setFillColor(sf::Color(241, 196, 15)); // Vàng: Ray chính
                } else {
                    trainRect.setFillColor(sf::Color(155, 89, 182)); // Tím: Ray phụ (Tránh tàu)
                }
                window.draw(trainRect);

                sf::Text trainText;
                trainText.setFont(font);
                trainText.setString(train->getTrainCode());
                trainText.setCharacterSize(12);
                trainText.setFillColor(sf::Color::White);
                trainText.setPosition(posX - 12.0f, posY - 22.0f);
                window.draw(trainText);
            }

            train = train->getNextTrain();
        }

        window.display();
    }
}
