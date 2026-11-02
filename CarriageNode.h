#pragma once

#include <string>
#include "SegmentTree.h"

struct Carriage {
    int carriageNo;
    std::string type;
    int totalSeats;
    SegmentTree<int>* seatSegments;
    int configuredSegments;

public:
    Carriage(int cNo, const std::string& cType, int seats)
        : carriageNo(cNo), type(cType), totalSeats(seats),
          seatSegments(nullptr), configuredSegments(0) {}

    ~Carriage() {
        delete[] seatSegments;
    }

    Carriage(const Carriage&) = delete;
    Carriage& operator=(const Carriage&) = delete;

    int getCarriageNo() const { return carriageNo; }
    std::string getType() const { return type; }
    int getTotalSeats() const { return totalSeats; }
    void configureSeatSegments(int segmentCount) {
        if (segmentCount <= 0) {
            delete[] seatSegments;
            seatSegments = nullptr;
            configuredSegments = 0;
            return;
        }
        if (segmentCount == configuredSegments) {
            for (int seat = 0; seat < totalSeats; ++seat) {
                seatSegments[seat].initialize(segmentCount, 1);
            }
            return;
        }

        delete[] seatSegments;
        seatSegments = nullptr;
        configuredSegments = segmentCount;
        if (totalSeats <= 0) {
            return;
        }
        seatSegments = new SegmentTree<int>[totalSeats];
        for (int seat = 0; seat < totalSeats; ++seat) {
            seatSegments[seat].initialize(configuredSegments, 1);
        }
    }

    bool isSeatAvailableOnSegments(int seatNo, int fromSegment, int toSegment) {
        if (!seatSegments || seatNo < 1 || seatNo > totalSeats) {
            return false;
        }
        return seatSegments[seatNo - 1].queryMin(fromSegment, toSegment) > 0;
    }

    void updateSeatSegments(int seatNo, int fromSegment, int toSegment, int delta) {
        if (seatSegments && seatNo >= 1 && seatNo <= totalSeats) {
            seatSegments[seatNo - 1].updateRange(fromSegment, toSegment, delta);
        }
    }
};

typedef Carriage CarriageNode;
