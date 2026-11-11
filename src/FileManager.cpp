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
#include <stdexcept>

using namespace std;

namespace {
int parseCommandInteger(const string& value) {
    string::size_type parsed = 0;
    const int number = stoi(value, &parsed);
    if (parsed != value.size()) throw invalid_argument("So khong hop le");
    return number;
}

double parseCommandDouble(const string& value) {
    string::size_type parsed = 0;
    const double number = stod(value, &parsed);
    if (parsed != value.size()) throw invalid_argument("So thuc khong hop le");
    return number;
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

        if (token == "CARRIAGE") {
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

bool FileManager::processInputCommands(const string& filename,
                                       StationList& stationList,
                                       TrainList& trainList,
                                       string& resultLog) {
    ifstream inFile(filename);
    if (!inFile.is_open()) {
        resultLog = "Khong mo duoc file lenh " + filename;
        return false;
    }

    bool changedStations = false;
    bool changedTrains = false;
    string line;
    int lineNumber = 0;
    resultLog.clear();
    while (getline(inFile, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;

        string fields[8];
        int fieldCount = 0;
        stringstream parser(line);
        while (fieldCount < 8 && getline(parser, fields[fieldCount], '|')) ++fieldCount;
        for (int i = 0; i < fieldCount; ++i) {
            if (!fields[i].empty() && fields[i].back() == '\r') fields[i].pop_back();
        }
        const string& command = fields[0];
        bool success = false;
        try {
            // Mỗi lệnh chỉ được áp dụng sau khi kiểm tra toàn bộ điều kiện liên quan.
            if (command == "ADD_STATION" && fieldCount == 5) {
                success = stationList.addStation(parseCommandInteger(fields[1]), fields[2],
                    parseCommandDouble(fields[3]), parseCommandInteger(fields[4]));
                changedStations = changedStations || success;
            } else if (command == "UPDATE_STATION" && fieldCount == 5) {
                const int id = parseCommandInteger(fields[1]);
                StationNode* station = stationList.findStationByID(id);
                const int tracks = parseCommandInteger(fields[4]);
                const double km = parseCommandDouble(fields[3]);
                const bool routeSafe = (!station || !trainList.hasStationStop(id) ||
                    km == station->getKmMarker()) &&
                    (!station || km == station->getKmMarker() ||
                     !trainList.hasMaintenanceBlockForStation(id));
                success = routeSafe && !trainList.hasTrackAbove(id, tracks) &&
                    !trainList.hasMaintenanceTrackAbove(id, tracks) &&
                    stationList.updateStation(id, fields[2], km, tracks);
                changedStations = changedStations || success;
            } else if (command == "DELETE_STATION" && fieldCount == 2) {
                const int id = parseCommandInteger(fields[1]);
                success = !trainList.hasStationStop(id) &&
                          !trainList.hasMaintenanceForStation(id) &&
                          stationList.removeStation(id);
                changedStations = changedStations || success;
            } else if (command == "ADD_TRAIN" && fieldCount >= 3 && fieldCount <= 4) {
                const int priority = fieldCount == 4 ? parseCommandInteger(fields[3]) : -1;
                success = trainList.addTrain(fields[1], parseCommandInteger(fields[2]), priority);
                changedTrains = changedTrains || success;
            } else if (command == "UPDATE_TRAIN" && fieldCount == 4) {
                success = trainList.updateTrain(fields[1], parseCommandInteger(fields[2]),
                    parseCommandInteger(fields[3]), &stationList);
                changedTrains = changedTrains || success;
            } else if (command == "DELETE_TRAIN" && fieldCount == 2) {
                success = trainList.removeTrain(fields[1]);
                changedTrains = changedTrains || success;
            } else if (command == "ADD_CARRIAGE" && fieldCount == 4) {
                success = trainList.addCarriageToTrain(fields[1],
                    parseCommandInteger(fields[2]), fields[3]);
                changedTrains = changedTrains || success;
            } else if (command == "UPDATE_CARRIAGE" && fieldCount == 4) {
                success = trainList.updateCarriage(fields[1],
                    parseCommandInteger(fields[2]), fields[3]);
                changedTrains = changedTrains || success;
            } else if (command == "DELETE_CARRIAGE" && fieldCount == 3) {
                const int carriageNo = parseCommandInteger(fields[2]);
                success = trainList.removeCarriage(fields[1], carriageNo);
                changedTrains = changedTrains || success;
            } else if (command == "ADD_STOP" && fieldCount == 6) {
                success = trainList.addStopScheduleToTrain(fields[1], parseCommandInteger(fields[2]),
                    parseCommandInteger(fields[3]), parseCommandInteger(fields[4]),
                    parseCommandInteger(fields[5]), &stationList);
                changedTrains = changedTrains || success;
            } else if (command == "UPDATE_STOP" && fieldCount == 6) {
                success = trainList.updateStopSchedule(fields[1], parseCommandInteger(fields[2]),
                    parseCommandInteger(fields[3]), parseCommandInteger(fields[4]),
                    parseCommandInteger(fields[5]), &stationList);
                changedTrains = changedTrains || success;
            } else if (command == "DELETE_STOP" && fieldCount == 3) {
                const int stationID = parseCommandInteger(fields[2]);
                success = trainList.removeStopSchedule(fields[1], stationID);
                changedTrains = changedTrains || success;
            } else if ((command == "CLOSE_STATION" && fieldCount == 4) ||
                       (command == "OPEN_STATION" && fieldCount == 4) ||
                       (command == "CLOSE_BLOCK" && fieldCount == 5) ||
                       (command == "OPEN_BLOCK" && fieldCount == 5) ||
                       (command == "CLOSE_TRACK" && fieldCount == 5) ||
                       (command == "OPEN_TRACK" && fieldCount == 5) ||
                       (command == "REPAIR_STATION" && fieldCount == 4) ||
                       (command == "REPAIR_BLOCK" && fieldCount == 5) ||
                       (command == "REPAIR_TRACK" && fieldCount == 5)) {
                const bool opening = command.rfind("OPEN_", 0) == 0;
                const string resource = command.find("STATION") != string::npos
                    ? "STATION" : (command.find("BLOCK") != string::npos ? "BLOCK" : "TRACK");
                const int first = parseCommandInteger(fields[1]);
                const int second = resource == "STATION" ? 0 : parseCommandInteger(fields[2]);
                const int startField = resource == "STATION" ? 2 : 3;
                const int endField = resource == "STATION" ? 3 : 4;
                const bool stationsExist = stationList.findStationByID(first) &&
                    (resource != "BLOCK" || stationList.findStationByID(second));
                bool adjacentBlock = resource != "BLOCK";
                if (resource == "BLOCK" && stationsExist) {
                    StationNode* firstStation = stationList.findStationByID(first);
                    StationNode* secondStation = stationList.findStationByID(second);
                    adjacentBlock = firstStation->getKmMarker() != secondStation->getKmMarker();
                    const double lowKm = firstStation->getKmMarker() < secondStation->getKmMarker()
                        ? firstStation->getKmMarker() : secondStation->getKmMarker();
                    const double highKm = firstStation->getKmMarker() > secondStation->getKmMarker()
                        ? firstStation->getKmMarker() : secondStation->getKmMarker();
                    for (LinkedList<StationNode*>::Node* station = stationList.getHead();
                         station; station = station->next) {
                        const double km = station->data->getKmMarker();
                        if (km > lowKm && km < highKm) adjacentBlock = false;
                    }
                }
                const bool trackExists = resource != "TRACK" ||
                    (stationList.findStationByID(first) &&
                     second <= stationList.findStationByID(first)->getMaxTracks());
                const int startMinute = parseCommandInteger(fields[startField]);
                const int endMinute = parseCommandInteger(fields[endField]);
                success = stationsExist && adjacentBlock && trackExists &&
                    (opening
                        ? trainList.removeMaintenance(resource, first, second,
                                                      startMinute, endMinute)
                        : trainList.addMaintenance(resource, first, second,
                                                   startMinute, endMinute));
                changedTrains = changedTrains || success;
            }
        } catch (const exception&) {
            success = false;
        }

        resultLog += "Dong " + to_string(lineNumber) + ": " + command +
            (success ? " [OK]\n" : " [TU CHOI - sai du lieu, khong ton tai hoac vi pham rang buoc]\n");
    }

    bool saved = true;
    if (changedStations && !saveStations("data/stations.txt", stationList)) {
        resultLog += "Loi: khong ghi duoc stations.txt\n";
        saved = false;
    }
    if (!changedStations && !changedTrains && resultLog.empty()) {
        resultLog = "Khong co lenh nao trong commands.txt.";
    }
    trainList.resolveConflictAndScheduleSiding(&stationList);
    if (!saveTrains("data/trains.txt", trainList)) {
        resultLog += "Loi: khong ghi duoc trains.txt\n";
        saved = false;
    }
    return saved;
}