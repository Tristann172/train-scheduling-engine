#include "FileManager.h"
#include "StationList.h"
#include "TrainList.h"
#include "TicketList.h"

#include "StationNode.h"
#include "TrainNode.h"
#include "CarriageNode.h"
#include "StopScheduleNode.h"
#include "TicketNode.h"

#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>

void FileManager::loadStations(const std::string& filename, StationList& stationList) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string idStr, name, kmStr, tracksStr;

        if (std::getline(ss, idStr, '|') &&
            std::getline(ss, name, '|') &&
            std::getline(ss, kmStr, '|') &&
            std::getline(ss, tracksStr, '|')) {
            stationList.addStation(std::stoi(idStr), name, std::stod(kmStr), std::stoi(tracksStr));
        }
    }
    file.close();
}

void FileManager::saveStations(const std::string& filename, const StationList& stationList) {
    std::ofstream file(filename.c_str());
    if (!file.is_open()) return;

    StationNode* curr = stationList.getHead();
    while (curr != nullptr) {
        file << curr->getStationID() << "|"
             << curr->getName() << "|"
             << curr->getKmMarker() << "|"
             << curr->getNumberOfTracks() << "\n";
        curr = curr->getNextStation();
    }
    file.close();
}

void FileManager::loadTrains(const std::string& filename, TrainList& trainList) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) return;

    std::string line;
    std::string currentTrainCode = "";

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string type;
        std::getline(ss, type, '|');

        if (type == "TRAIN") {
            std::string code, dirStr;
            std::getline(ss, code, '|');
            std::getline(ss, dirStr, '|');
            currentTrainCode = code;
            trainList.addTrain(code, std::stoi(dirStr));
        } 
        else if (type == "CARRIAGE" && !currentTrainCode.empty()) {
            std::string cNoStr, cType, seatsStr;
            std::getline(ss, cNoStr, '|');
            std::getline(ss, cType, '|');
            std::getline(ss, seatsStr, '|');
            trainList.addCarriageToTrain(currentTrainCode, std::stoi(cNoStr), cType, std::stoi(seatsStr));
        } 
        else if (type == "STOP" && !currentTrainCode.empty()) {
            std::string stIDStr, arrStr, depStr, trackStr;
            std::getline(ss, stIDStr, '|');
            std::getline(ss, arrStr, '|');
            std::getline(ss, depStr, '|');
            std::getline(ss, trackStr, '|');
            trainList.addStopScheduleToTrain(currentTrainCode, std::stoi(stIDStr), std::stoi(arrStr), std::stoi(depStr), std::stoi(trackStr));
        }
    }
    file.close();
}

void FileManager::saveTrains(const std::string& filename, const TrainList& trainList) {
    std::ofstream file(filename.c_str());
    if (!file.is_open()) return;

    TrainNode* train = trainList.getHead();
    while (train != nullptr) {
        file << "TRAIN|" << train->getTrainCode() << "|" << train->getDirection() << "\n";

        CarriageNode* carriage = train->getHeadCarriage();
        while (carriage != nullptr) {
            file << "CARRIAGE|" << carriage->getCarriageNo() << "|"
                 << carriage->getType() << "|" << carriage->getTotalSeats() << "\n";
            carriage = carriage->getNextCarriage();
        }

        StopScheduleNode* stop = train->getHeadSchedule();
        while (stop != nullptr) {
            file << "STOP|" << stop->getStationID() << "|"
                 << stop->getArrivalMinute() << "|"
                 << stop->getDepartureMinute() << "|"
                 << stop->getTrackUsed() << "\n";
            stop = stop->getNextStop();
        }

        train = train->getNextTrain();
    }
    file.close();
}

void FileManager::loadTickets(const std::string& filename, TicketList& ticketList) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string tID, code, cNoStr, sNoStr, fromStr, toStr;

        if (std::getline(ss, tID, '|') &&
            std::getline(ss, code, '|') &&
            std::getline(ss, cNoStr, '|') &&
            std::getline(ss, sNoStr, '|') &&
            std::getline(ss, fromStr, '|') &&
            std::getline(ss, toStr, '|')) {
            ticketList.addTicket(tID, code, std::stoi(cNoStr), std::stoi(sNoStr), std::stoi(fromStr), std::stoi(toStr));
        }
    }
    file.close();
}

void FileManager::saveTickets(const std::string& filename, const TicketList& ticketList) {
    std::ofstream file(filename.c_str());
    if (!file.is_open()) return;

    TicketNode* curr = ticketList.getHead();
    while (curr != nullptr) {
        file << curr->getTicketID() << "|"
             << curr->getTrainCode() << "|"
             << curr->getCarriageNo() << "|"
             << curr->getSeatNo() << "|"
             << curr->getFromStationID() << "|"
             << curr->getToStationID() << "\n";
        curr = curr->getNextTicket();
    }
    file.close();
}
