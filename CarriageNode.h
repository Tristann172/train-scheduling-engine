#pragma once

#include <string>
#include "SegmentTree.h"

// Quản lý thông tin và trạng thái đặt chỗ của một toa tàu
struct Carriage {
    int carriageNo;                         // Số hiệu toa
    std::string type;                       // Loại toa (ghế ngồi, giường nằm...)
    int totalSeats;                         // Tổng số ghế trong toa
    SegmentTree<int>* seatSegments;         // Mảng Segment Tree quản lý tình trạng từng ghế theo chặng
    int configuredSegments;                 // Số lượng chặng đường hiện tại

public:
    // Khởi tạo thông tin cơ bản của toa
    Carriage(int cNo, const std::string& cType, int seats)
        : carriageNo(cNo), type(cType), totalSeats(seats),
          seatSegments(nullptr), configuredSegments(0) {}

    // Giải phóng mảng Segment Tree trên Heap
    ~Carriage() {
        delete[] seatSegments;
    }

    // Chặn sao chép để tránh lỗi double-free con trỏ seatSegments
    Carriage(const Carriage&) = delete;
    Carriage& operator=(const Carriage&) = delete;

    // Lấy thông tin toa
    int getCarriageNo() const { return carriageNo; }
    std::string getType() const { return type; }
    int getTotalSeats() const { return totalSeats; }

    // Khởi tạo/cấp phát lại Segment Tree cho từng ghế theo số lượng chặng ga
    void configureSeatSegments(int segmentCount) {
        if (segmentCount <= 0) {
            delete[] seatSegments;
            seatSegments = nullptr;
            configuredSegments = 0;
            return;
        }
        // Giữ nguyên bộ nhớ, chỉ reset trạng thái nếu số chặng không đổi
        if (segmentCount == configuredSegments) {
            for (int seat = 0; seat < totalSeats; ++seat) {
                seatSegments[seat].initialize(segmentCount, 1);
            }
            return;
        }

        // Cấp phát mảng Segment Tree mới cho tất cả các ghế
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

    // Kiểm tra ghế có còn trống trên toàn bộ đoạn chặng [fromSegment, toSegment] hay không
    bool isSeatAvailableOnSegments(int seatNo, int fromSegment, int toSegment) {
        if (!seatSegments || seatNo < 1 || seatNo > totalSeats) {
            return false;
        }
        return seatSegments[seatNo - 1].queryMin(fromSegment, toSegment) > 0;
    }

    // Cập nhật trạng thái ghế trên đoạn chặng (delta = -1 khi đặt vé, +1 khi hủy vé)
    void updateSeatSegments(int seatNo, int fromSegment, int toSegment, int delta) {
        if (seatSegments && seatNo >= 1 && seatNo <= totalSeats) {
            seatSegments[seatNo - 1].updateRange(fromSegment, toSegment, delta);
        }
    }
};

// Bí danh dùng khi đưa toa vào danh sách liên kết
typedef Carriage CarriageNode;