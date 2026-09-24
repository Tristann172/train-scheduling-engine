#include "TicketList.h"
#include "StationList.h" // Bắt buộc include tại đây để dùng hàm findStationByID
#include <iostream>

TicketList::TicketList() : head(nullptr) {}

// Hàm dọn dẹp bộ nhớ Heap triệt để
void TicketList::clear() {
    while (head != nullptr) {
        TicketNode* temp = head;
        head = head->getNextTicket();
        delete temp; // Giải phóng từng Node vé
    }
    head = nullptr;
}

TicketList::~TicketList() {
    clear(); // Gọi hàm clear để đáp ứng tiêu chuẩn Zero Leaks
}

TicketNode* TicketList::getHead() const {
    return head;
}

// Chèn Node vé mới vào đầu danh sách liên kết đơn (O(1))
void TicketList::addTicket(const std::string& ticketID, const std::string& trainCode,
                           int carriageNo, int seatNo, int fromID, int toID) 
{
    TicketNode* newTicket = new TicketNode(ticketID, trainCode, carriageNo, seatNo, fromID, toID);
    newTicket->setNextTicket(head);
    head = newTicket;
}

// Thuật toán kiểm tra giao thoa chặng vé (Đỗ Hữu Minh Trí phụ trách)
bool TicketList::isSeatAvailable(StationList* stationList, const std::string& trainCode,
                                 int carriageNo, int seatNo, int newFromID, int newToID) const 
{
    // Kiểm tra an toàn cho con trỏ ngoại lai
    if (stationList == nullptr) {
        std::cout << "[Loi] Danh sach ga khong ton tai!\n";
        return false; 
    }

    // Tra cứu mốc Km của khách mới từ danh sách ga của Gia Huy
    StationNode* newFromStation = stationList->findStationByID(newFromID);
    StationNode* newToStation = stationList->findStationByID(newToID);
    
    if (newFromStation == nullptr || newToStation == nullptr) {
        std::cout << "[Loi] Ma ga khong hop le!\n";
        return false;
    }

    double newFromKm = newFromStation->getKmMarker();
    double newToKm = newToStation->getKmMarker();
    
    // Quy chuẩn min-max để không phân biệt chiều tàu chạy (Bắc-Nam hay Nam-Bắc)
    double newMinKm = (newFromKm < newToKm) ? newFromKm : newToKm;
    double newMaxKm = (newFromKm < newToKm) ? newToKm : newFromKm;

    TicketNode* curr = head;
    while (curr != nullptr) {
        // Chỉ xét những vé cũ có chung mã tàu, số toa và số ghế
        if (curr->getTrainCode() == trainCode && 
            curr->getCarriageNo() == carriageNo && 
            curr->getSeatNo() == seatNo) 
        {
            StationNode* oldFromStation = stationList->findStationByID(curr->getFromStationID());
            StationNode* oldToStation = stationList->findStationByID(curr->getToStationID());

            if (oldFromStation != nullptr && oldToStation != nullptr) {
                double oldFromKm = oldFromStation->getKmMarker();
                double oldToKm = oldToStation->getKmMarker();
                
                double oldMinKm = (oldFromKm < oldToKm) ? oldFromKm : oldToKm;
                double oldMaxKm = (oldFromKm < oldToKm) ? oldToKm : oldFromKm;

                // THUẬT TOÁN KIỂM TRA GIAO NHAU (Overlap)
                // Hai đoạn thẳng giao nhau nếu: max(min1, min2) < min(max1, max2)
                double overlapStart = (oldMinKm > newMinKm) ? oldMinKm : newMinKm;
                double overlapEnd = (oldMaxKm < newMaxKm) ? oldMaxKm : newMaxKm;

                if (overlapStart < overlapEnd) {
                    return false; // Phát hiện chặng bị trùng, từ chối cấp vé
                }
            }
        }
        curr = curr->getNextTicket(); // Di chuyển con trỏ tuần tự
    }
    
    return true; // Không có vé nào trùng chặng, ghế hoàn toàn khả dụng
}