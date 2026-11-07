#pragma once

#include <string>
// Thông tin toa tàu phục vụ quản lý đoàn tàu và lịch trình.
struct Carriage {
    int carriageNo;                         // Số hiệu toa
    std::string type;                       // Loại toa (ghế ngồi, giường nằm...)

public:
    Carriage(int cNo, const std::string& cType)
        : carriageNo(cNo), type(cType) {}

    // Lấy thông tin toa
    int getCarriageNo() const { return carriageNo; }
    std::string getType() const { return type; }
    void setType(const std::string& newType) { type = newType; }
};

// Bí danh dùng khi đưa toa vào danh sách liên kết
typedef Carriage CarriageNode;