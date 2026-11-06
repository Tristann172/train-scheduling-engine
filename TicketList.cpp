#include "TicketList.h"
#include "StationList.h"
#include "TrainList.h"
#include "StationNode.h"
#include "TrainNode.h"
#include "StopScheduleNode.h"
#include "CarriageNode.h"
#include <iostream>

using namespace std;

namespace {
int findScheduleIndex(TrainNode* train, int stationID) {
    int index = 0;
    for (LinkedList<StopScheduleNode*>::Node* stopNode =
             train ? train->getHeadSchedule() : nullptr;
         stopNode != nullptr; stopNode = stopNode->next, ++index) {
        StopScheduleNode* stop = stopNode->data;
        if (stop->getStationID() == stationID) {
            return index;
        }
    }
    return -1;
}

int countScheduleStops(TrainNode* train) {
    int count = 0;
    for (LinkedList<StopScheduleNode*>::Node* stop =
             train ? train->getHeadSchedule() : nullptr;
         stop != nullptr; stop = stop->next) {
        ++count;
    }
    return count;
}
}

TicketList::TicketList()
    : tickets(), ticketsByID(), occupancyRevision(0),
      builtOccupancyRevision(~0UL), builtTrainRevision(~0UL),
      occupancyTrainList(nullptr) {}

TicketList::~TicketList() {
    clear();
}

void TicketList::clear() {
    LinkedList<TicketNode*>::Node* ticket = tickets.getHead();
    while (ticket) {
        delete ticket->data;
        ticket = ticket->next;
    }
    tickets.clear();
    ticketsByID.clear();
    ++occupancyRevision;
}

// Chèn vé mới vào cuối danh sách liên kết với độ phức tạp O(1)
void TicketList::addTicket(const string& ticketID, const string& trainCode, 
                           int carriageNo, int seatNo, int fromID, int toID) {
    TicketNode* newNode = new TicketNode(ticketID, trainCode, carriageNo, seatNo, fromID, toID);
    const bool cacheWasCurrent =
        builtOccupancyRevision == occupancyRevision && occupancyTrainList &&
        builtTrainRevision == occupancyTrainList->getStructureRevision();
    tickets.pushBack(newNode);
    if (!ticketsByID.find(ticketID)) {
        ticketsByID.insert(ticketID, newNode);
    }
    ++occupancyRevision;
    if (cacheWasCurrent) {
        updateSeatOccupancyForTicket(newNode, -1);
        builtOccupancyRevision = occupancyRevision;
    }
}

TicketNode* TicketList::findTicketByID(const string& ticketID) const {
    TicketNode* const* ticket = ticketsByID.find(ticketID);
    return ticket ? *ticket : nullptr;
}

bool TicketList::cancelTicket(const string& ticketID) {
    TicketNode* const* indexedTicket = ticketsByID.find(ticketID);
    if (!indexedTicket) return false;

    LinkedList<TicketNode*>::Node* target = tickets.getHead();
    while (target && target->data != *indexedTicket) {
        target = target->next;
    }
    if (!target) return false;

    const bool cacheWasCurrent =
        builtOccupancyRevision == occupancyRevision && occupancyTrainList &&
        builtTrainRevision == occupancyTrainList->getStructureRevision();
    TicketNode* ticket = target->data;
    if (cacheWasCurrent) updateSeatOccupancyForTicket(ticket, 1);
    tickets.removeNode(target);
    delete ticket;
    ticketsByID.remove(ticketID);
    for (LinkedList<TicketNode*>::Node* remaining = tickets.getHead();
         remaining; remaining = remaining->next) {
        if (remaining->data->getTicketID() == ticketID) {
            ticketsByID.insert(ticketID, remaining->data);
            break;
        }
    }
    ++occupancyRevision;
    if (cacheWasCurrent) builtOccupancyRevision = occupancyRevision;
    return true;
}

void TicketList::updateSeatOccupancyForTicket(const TicketNode* ticket,
                                               int delta) const {
    if (!ticket || !occupancyTrainList) return;
    TrainNode* train = occupancyTrainList->findTrainByCode(ticket->getTrainCode());
    if (!train) return;

    const int fromIndex = findScheduleIndex(train, ticket->getFromStationID());
    const int toIndex = findScheduleIndex(train, ticket->getToStationID());
    CarriageNode* carriage = train->findCarriage(ticket->getCarriageNo());
    if (carriage && fromIndex >= 0 && toIndex > fromIndex &&
        ticket->getSeatNo() >= 1 && ticket->getSeatNo() <= carriage->getTotalSeats()) {
        carriage->updateSeatSegments(ticket->getSeatNo(), fromIndex,
                                     toIndex - 1, delta);
    }
}

void TicketList::ensureSeatOccupancy(TrainList* trainList) const {
    if (!trainList ||
        (builtOccupancyRevision == occupancyRevision && occupancyTrainList == trainList &&
         builtTrainRevision == trainList->getStructureRevision())) {
        return;
    }

    // Reset each seat's route tree once, then replay tickets in one pass.
    for (LinkedList<TrainNode*>::Node* trainNode = trainList->getHead();
         trainNode; trainNode = trainNode->next) {
        TrainNode* train = trainNode->data;
        const int segmentCount = countScheduleStops(train) - 1;
        for (LinkedList<CarriageNode*>::Node* carriageNode =
                 train->getHeadCarriage(); carriageNode;
             carriageNode = carriageNode->next) {
            CarriageNode* carriage = carriageNode->data;
            carriage->configureSeatSegments(segmentCount);
        }
    }

    for (LinkedList<TicketNode*>::Node* ticketNode = tickets.getHead();
         ticketNode; ticketNode = ticketNode->next) {
        TicketNode* ticket = ticketNode->data;
        TrainNode* train = trainList->findTrainByCode(ticket->getTrainCode());
        if (!train) continue;

        const int fromIndex = findScheduleIndex(train, ticket->getFromStationID());
        const int toIndex = findScheduleIndex(train, ticket->getToStationID());
        CarriageNode* carriage = train->findCarriage(ticket->getCarriageNo());
        if (carriage && fromIndex >= 0 && toIndex > fromIndex &&
            ticket->getSeatNo() >= 1 && ticket->getSeatNo() <= carriage->getTotalSeats()) {
            carriage->updateSeatSegments(ticket->getSeatNo(), fromIndex, toIndex - 1, -1);
        }
    }

    occupancyTrainList = trainList;
    builtOccupancyRevision = occupancyRevision;
    builtTrainRevision = trainList->getStructureRevision();
}

bool TicketList::checkCarriageSeats(StationList* stationList, TrainList* trainList,
                                    const string& trainCode, int carriageNo,
                                    int newFromID, int newToID,
                                    bool seatFreeOut[], int maxSeats) const 
{
    if (!stationList || !trainList || !seatFreeOut ||
        newFromID == newToID || maxSeats <= 0) return false;

    StationNode* stNewFrom = stationList->findStationByID(newFromID);
    StationNode* stNewTo   = stationList->findStationByID(newToID);
    if (!stNewFrom || !stNewTo) return false;

    TrainNode* train = trainList->findTrainByCode(trainCode);
    if (!train) return false;

    const int fromIndex = findScheduleIndex(train, newFromID);
    const int toIndex = findScheduleIndex(train, newToID);
    CarriageNode* carriage = train->findCarriage(carriageNo);
    if (!carriage || fromIndex < 0 || toIndex <= fromIndex ||
        maxSeats > carriage->getTotalSeats()) return false;

    ensureSeatOccupancy(trainList);
    for (int seat = 1; seat <= maxSeats; ++seat) {
        seatFreeOut[seat] =
            carriage->isSeatAvailableOnSegments(seat, fromIndex, toIndex - 1);
    }

    return true;
}

// ==================== KIỂM TRA ĐƠN LẺ KHI ĐẶT VÉ (SEGMENT BOOKING) ====================
bool TicketList::isSeatAvailable(StationList* stationList, TrainList* trainList, const string& trainCode, 
                                 int carriageNo, int seatNo, int newFromID, int newToID) const 
{
    if (!stationList || !trainList || newFromID == newToID) return false;

    StationNode* stNewFrom = stationList->findStationByID(newFromID);
    StationNode* stNewTo   = stationList->findStationByID(newToID);
    if (!stNewFrom || !stNewTo) return false;

    TrainNode* train = trainList->findTrainByCode(trainCode);
    if (!train) return false;

    const int fromIndex = findScheduleIndex(train, newFromID);
    const int toIndex = findScheduleIndex(train, newToID);
    CarriageNode* carriage = train->findCarriage(carriageNo);
    if (!carriage || seatNo <= 0 || seatNo > carriage->getTotalSeats() ||
        fromIndex < 0 || toIndex <= fromIndex) return false;

    ensureSeatOccupancy(trainList);
    return carriage->isSeatAvailableOnSegments(seatNo, fromIndex, toIndex - 1);
}