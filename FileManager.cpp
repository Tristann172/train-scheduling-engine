#include "FileManager.h"
#include "StationList.h"
#include "TrainList.h"
#include "TicketList.h"

#include "StationNode.h"
#include "TrainNode.h"
#include "CarriageNode.h"
#include "StopScheduleNode.h"
#include "TicketNode.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

// ==================== STATIONS ====================
void FileManager::loadStations(const string& filename, StationList& stationList) {
    ifstream inFile(filename);
    if (!inFile.is_open()) return;

    string line;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string strID, name, strKm, strTracks;

        if (getline(ss, strID, '|') && getline(ss, name, '|') &&
            getline(ss, strKm, '|') && getline(ss, strTracks, '|')) {
            stationList.addStation(stoi(strID), name, stod(strKm), stoi(strTracks));
        }
    }
    inFile.close();
}

void FileManager::saveStations(const string& filename, const StationList& stationList) {
    ofstream outFile(filename);
    if (!outFile.is_open()) return;

    StationNode* st = stationList.getHead();
    while (st != nullptr) {
        outFile << st->getStationID() << "|"
                << st->getName() << "|"
                << st->getKmMarker() << "|"
                << st->getMaxTracks() << "\n";
        st = st->getNextStation();
    }
    outFile.close();
}

// ==================== TRAINS ====================
void FileManager::loadTrains(const string& filename, TrainList& trainList) {
    ifstream inFile(filename);
    if (!inFile.is_open()) return;

    string line;
    TrainNode* currentTrain = nullptr;

    while (getline(inFile, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        string token;
        getline(ss, token, '|');

        if (token == "CARRIAGE") {
            if (currentTrain) {
                string strNo, type, strSeats;
                if (getline(ss, strNo, '|') && getline(ss, type, '|') && getline(ss, strSeats, '|')) {
                    currentTrain->addCarriage(stoi(strNo), type, stoi(strSeats));
                }
            }
        } 
        else if (token == "STOP") {
            if (currentTrain) {
                string strStID, strArr, strDep, strTrack;
                if (getline(ss, strStID, '|') && getline(ss, strArr, '|') &&
                    getline(ss, strDep, '|') && getline(ss, strTrack, '|')) {
                    currentTrain->addSchedule(stoi(strStID), stoi(strArr), stoi(strDep), stoi(strTrack));
                }
            }
        } 
        else {
            // Định dạng dòng đầu: Mã Tàu | Hướng chạy [ | Độ ưu tiên ]
            string trainCode = token;
            string strDir, strPrio;

            if (getline(ss, strDir, '|')) {
                int prio = -1; // -1: TrainNode sẽ tự nhận diện theo tiền tố mã tàu
                
                // Đọc cột thứ 3 (nếu file có lưu sẵn độ ưu tiên)
                if (getline(ss, strPrio, '|') && !strPrio.empty()) {
                    try {
                        prio = stoi(strPrio);
                    } catch (...) {
                        prio = -1;
                    }
                }

                trainList.addTrain(trainCode, stoi(strDir), prio);
                currentTrain = trainList.findTrainByCode(trainCode);
            }
        }
    }
    inFile.close();
}

void FileManager::saveTrains(const string& filename, const TrainList& trainList) {
    ofstream outFile(filename);
    if (!outFile.is_open()) return;

    TrainNode* tr = trainList.getHead();
    while (tr != nullptr) {
        // Lưu kèm độ ưu tiên vào cột thứ 3: Mã Tàu | Hướng chạy | Độ ưu tiên
        outFile << tr->getTrainCode() << "|" 
                << tr->getDirection() << "|" 
                << tr->getPriority() << "\n";

        CarriageNode* c = tr->getHeadCarriage();
        while (c != nullptr) {
            outFile << "CARRIAGE|" << c->getCarriageNo() << "|" << c->getType() << "|" << c->getTotalSeats() << "\n";
            c = c->getNextCarriage();
        }

        StopScheduleNode* s = tr->getHeadSchedule();
        while (s != nullptr) {
            outFile << "STOP|" << s->getStationID() << "|" << s->getArrivalMinute() << "|" << s->getDepartureMinute() << "|" << s->getTrackUsed() << "\n";
            s = s->getNextStop();
        }

        tr = tr->getNextTrain();
    }
    outFile.close();
}

// ==================== TICKETS ====================
void FileManager::loadTickets(const string& filename, TicketList& ticketList) {
    ifstream inFile(filename);
    if (!inFile.is_open()) return;

    string line;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string tID, trCode, strCNo, strSNo, strFrom, strTo;

        if (getline(ss, tID, '|') && getline(ss, trCode, '|') &&
            getline(ss, strCNo, '|') && getline(ss, strSNo, '|') &&
            getline(ss, strFrom, '|') && getline(ss, strTo, '|')) {
            
            ticketList.addTicket(tID, trCode, stoi(strCNo), stoi(strSNo), stoi(strFrom), stoi(strTo));
        }
    }
    inFile.close();
}

void FileManager::saveTickets(const string& filename, const TicketList& ticketList) {
    ofstream outFile(filename);
    if (!outFile.is_open()) return;

    TicketNode* tk = ticketList.getHead();
    while (tk != nullptr) {
        outFile << tk->getTicketID() << "|"
                << tk->getTrainCode() << "|"
                << tk->getCarriageNo() << "|"
                << tk->getSeatNo() << "|"
                << tk->getFromStationID() << "|"
                << tk->getToStationID() << "\n";
        tk = tk->getNextTicket();
    }
    outFile.close();
}