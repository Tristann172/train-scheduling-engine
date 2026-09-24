#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include "StationList.h"
#include "TrainList.h"
#include "TicketList.h"

class FileManager {
public:
    static void loadStations(const std::string& filename, StationList& stationList) {
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

    static void loadTrains(const std::string& filename, TrainList& trainList) {
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

    static void saveTickets(const std::string& filename, const TicketList& ticketList) {
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

    static void loadTickets(const std::string& filename, TicketList& ticketList) {
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
};