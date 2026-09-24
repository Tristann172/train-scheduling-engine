#pragma once
#include <string>
#include "TicketNode.h"

class StationList; 
class StationNode;

class TicketList {
private:
    TicketNode* head;

public:
    TicketList();
    
    ~TicketList();
    void clear();

    TicketNode* getHead() const;

    // Thêm vé mới (Cấp phát động)
    void addTicket(const std::string& ticketID, const std::string& trainCode,
                   int carriageNo, int seatNo, int fromID, int toID);

    // Nghiệp vụ cốt lõi Tuần 5: Thuật toán kiểm tra khả dụng ghế theo chặng
    // Bắt buộc truyền tham số dạng con trỏ StationList*
    bool isSeatAvailable(StationList* stationList, const std::string& trainCode,
                         int carriageNo, int seatNo, int newFromID, int newToID) const;
};