#include "FileManager.h"
#include "StationList.h"
#include "TrainList.h"

#include "StationNode.h"
#include "TrainNode.h"
#include "CarriageNode.h"
#include "StopScheduleNode.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

namespace {
const char* maintenanceResourceName(int resourceType) {
    if (resourceType == 0) return "STATION";
    if (resourceType == 1) return "BLOCK";
    return "TRACK";
}
}

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

bool FileManager::saveStations(const string& filename, const StationList& stationList) {
    ofstream outFile(filename);
    if (!outFile.is_open()) return false;

    for (LinkedList<StationNode*>::Node* station = stationList.getHead();
         station != nullptr; station = station->next) {
        StationNode* st = station->data;
        outFile << st->getStationID() << "|"
                << st->getName() << "|"
                << st->getKmMarker() << "|"
                << st->getMaxTracks() << "\n";
    }
    outFile.close();
    return !outFile.fail();
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

        if (token == "MAINTENANCE") {
            string resource;
            string strFirst;
            string strSecond;
            string strStart;
            string strEnd;
            if (getline(ss, resource, '|') && getline(ss, strFirst, '|') &&
                getline(ss, strSecond, '|') && getline(ss, strStart, '|') &&
                getline(ss, strEnd, '|')) {
                trainList.addMaintenance(resource, stoi(strFirst), stoi(strSecond),
                                          stoi(strStart), stoi(strEnd));
            }
        } else if (token == "CARRIAGE") {
            if (currentTrain) {
                string strNo, type;
                if (getline(ss, strNo, '|') && getline(ss, type, '|')) {
                    currentTrain->addCarriage(stoi(strNo), type);
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
                int prio = -1;
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

bool FileManager::saveTrains(const string& filename, const TrainList& trainList) {
    ofstream outFile(filename);
    if (!outFile.is_open()) return false;

    for (int i = 0; i < trainList.getMaintenanceCount(); ++i) {
        const MaintenanceWindow& window = trainList.getMaintenance(i);
        outFile << "MAINTENANCE|" << maintenanceResourceName(window.resourceType)
                << "|" << window.firstID
                << "|" << window.secondID
                << "|" << window.startMinute
                << "|" << window.endMinute << "\n";
    }

    for (LinkedList<TrainNode*>::Node* trainNode = trainList.getHead();
         trainNode != nullptr; trainNode = trainNode->next) {
        TrainNode* tr = trainNode->data;
        // Lưu kèm độ ưu tiên vào cột thứ 3: Mã Tàu | Hướng chạy | Độ ưu tiên
        outFile << tr->getTrainCode() << "|" 
                << tr->getDirection() << "|" 
                << tr->getPriority() << "\n";

        for (LinkedList<CarriageNode*>::Node* carriage = tr->getHeadCarriage();
             carriage != nullptr; carriage = carriage->next) {
            CarriageNode* c = carriage->data;
            outFile << "CARRIAGE|" << c->getCarriageNo() << "|" << c->getType() << "\n";
        }

        for (LinkedList<StopScheduleNode*>::Node* stop = tr->getHeadSchedule();
             stop != nullptr; stop = stop->next) {
            StopScheduleNode* s = stop->data;
            outFile << "STOP|" << s->getStationID() << "|" << s->getArrivalMinute() << "|" << s->getDepartureMinute() << "|" << s->getTrackUsed() << "\n";
        }
    }
    outFile.close();
    return !outFile.fail();
}