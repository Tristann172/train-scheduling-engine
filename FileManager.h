#pragma once

#include <string>

class StationList;
class TrainList;
class TicketList;

class FileManager {
public:
    static void loadStations(const std::string& filename, StationList& stationList);
    static void saveStations(const std::string& filename, const StationList& stationList);

    static void loadTrains(const std::string& filename, TrainList& trainList);
    static void saveTrains(const std::string& filename, const TrainList& trainList);

    static void loadTickets(const std::string& filename, TicketList& ticketList);
    static void saveTickets(const std::string& filename, const TicketList& ticketList);
};
