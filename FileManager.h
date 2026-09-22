#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include "StationList.h"
#include "TrainList.h"

using namespace std;

class FileManager {
public:
    static void loadStations(const string& filename, StationList& stationList) {
        ifstream file(filename.c_str());
        if (!file.is_open()) return;

        string line;
        while (getline(file, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string idStr, name, kmStr, tracksStr;

            if (getline(ss, idStr, '|') &&
                getline(ss, name, '|') &&
                getline(ss, kmStr, '|') &&
                getline(ss, tracksStr, '|')) {
                stationList.addStation(stoi(idStr), name, stod(kmStr), stoi(tracksStr));
            }
        }
        file.close();
    }

    static void loadTrains(const string& filename, TrainList& trainList) {
        ifstream file(filename);
        if (!file.is_open()) return;

        string line;
        string currentTrainCode = "";

        while (getline(file, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string type;
            getline(ss, type, '|');

            if (type == "TRAIN") {
                string code, dirStr;
                getline(ss, code, '|');
                getline(ss, dirStr, '|');
                currentTrainCode = code;
                trainList.addTrain(code, stoi(dirStr));
            } 
            else if (type == "CARRIAGE" && !currentTrainCode.empty()) {
                string cNoStr, cType, seatsStr;
                getline(ss, cNoStr, '|');
                getline(ss, cType, '|');
                getline(ss, seatsStr, '|');
                trainList.addCarriageToTrain(currentTrainCode, stoi(cNoStr), cType, stoi(seatsStr));
            } 
            else if (type == "STOP" && !currentTrainCode.empty()) {
                string stIDStr, arrStr, depStr, trackStr;
                getline(ss, stIDStr, '|');
                getline(ss, arrStr, '|');
                getline(ss, depStr, '|');
                getline(ss, trackStr, '|');
                trainList.addStopScheduleToTrain(currentTrainCode, stoi(stIDStr), stoi(arrStr), stoi(depStr), stoi(trackStr));
            }
        }
        file.close();
    }
};
