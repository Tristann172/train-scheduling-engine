#pragma once

#include <string>

class StationList;
class TrainList;
class TicketList;

class FileManager {
public:
    // 1. Quan ly du lieu ga (stations.txt)
    static void loadStations(const std::string& filename, StationList& stationList);
    static void saveStations(const std::string& filename, const StationList& stationList);

    // 2. Quan ly du lieu tau & lich trinh (trains.txt)
    static void loadTrains(const std::string& filename, TrainList& trainList);
    static void saveTrains(const std::string& filename, const TrainList& trainList);

    // 3. Quan ly du lieu ve (tickets.txt)
    static void loadTickets(const std::string& filename, TicketList& ticketList);
    static void saveTickets(const std::string& filename, const TicketList& ticketList);
};
