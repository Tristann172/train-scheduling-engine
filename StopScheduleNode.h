#pragma once

struct StopSchedule {
    int stationID;
    int arrivalMinute;  
    int departureMinute; 
    int trackUsed;     
public:
    StopSchedule(int stID, int arr, int dep, int track)
        : stationID(stID), arrivalMinute(arr), departureMinute(dep), trackUsed(track) {}

    int getStationID() const { return stationID; }
    int getArrivalMinute() const { return arrivalMinute; }
    int getDepartureMinute() const { return departureMinute; }
    int getTrackUsed() const { return trackUsed; }
    void setArrivalMinute(int arr) { arrivalMinute = arr; }
    void setDepartureMinute(int dep) { departureMinute = dep; }
    void setTrackUsed(int track) { trackUsed = track; }
};

typedef StopSchedule StopScheduleNode;