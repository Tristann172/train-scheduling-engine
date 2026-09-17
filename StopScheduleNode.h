#ifndef STOP_SCHEDULE_NODE_H
#define STOP_SCHEDULE_NODE_H

class StopScheduleNode {
private:
    int stationID;
    int arrivalMinute;  
    int departureMinute; 
    int trackUsed;     
    StopScheduleNode* nextStop;

public:
    StopScheduleNode(int stID, int arr, int dep, int track)
        : stationID(stID), arrivalMinute(arr), departureMinute(dep), trackUsed(track), nextStop(NULL) {}

    int getStationID() const { return stationID; }
    int getArrivalMinute() const { return arrivalMinute; }
    int getDepartureMinute() const { return departureMinute; }
    int getTrackUsed() const { return trackUsed; }
    StopScheduleNode* getNextStop() const { return nextStop; }

    void setArrivalMinute(int arr) { arrivalMinute = arr; }
    void setDepartureMinute(int dep) { departureMinute = dep; }
    void setTrackUsed(int track) { trackUsed = track; }
    void setNextStop(StopScheduleNode* next) { nextStop = next; }
};

#endif
