#ifndef CARRIAGE_NODE_H
#define CARRIAGE_NODE_H

#include <string>

class CarriageNode {
private:
    int carriageNo;
    std::string type; // "Ngoi", "Giuong"
    int totalSeats;
    CarriageNode* nextCarriage;

public:
    CarriageNode(int cNo, const std::string& cType, int seats)
        : carriageNo(cNo), type(cType), totalSeats(seats), nextCarriage(NULL) {}

    int getCarriageNo() const { return carriageNo; }
    std::string getType() const { return type; }
    int getTotalSeats() const { return totalSeats; }
    CarriageNode* getNextCarriage() const { return nextCarriage; }

    void setNextCarriage(CarriageNode* next) { nextCarriage = next; }
};

#endif
