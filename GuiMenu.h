#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

class StationList;
class TrainList;
class TicketList;

// Cấu trúc đại diện cho một Nút bấm (Button) trong Menu
struct GuiButton {
    sf::RectangleShape shape;
    sf::Text text;
    int actionID; // ID tương ứng với từng chức năng
    bool isHovered;

    GuiButton(float x, float y, float width, float height, const std::string& label, sf::Font& font, int id) 
        : actionID(id), isHovered(false) 
    {
        shape.setPosition(x, y);
        shape.setSize(sf::Vector2f(width, height));
        shape.setFillColor(sf::Color(41, 128, 185)); // Màu xanh dương mặc định
        shape.setOutlineThickness(2.0f);
        shape.setOutlineColor(sf::Color(52, 152, 219));

        text.setFont(font);
        text.setString(label);
        text.setCharacterSize(16);
        text.setFillColor(sf::Color::White);
        
        // Căn giữa chữ trong nút bấm
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setPosition(
            x + (width - textBounds.width) / 2.0f - textBounds.left,
            y + (height - textBounds.height) / 2.0f - textBounds.top
        );
    }

    void updateHover(const sf::Vector2i& mousePos) {
        if (shape.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
            isHovered = true;
            shape.setFillColor(sf::Color(52, 152, 219)); // Đổi màu sáng hơn khi Rê chuột vào
            shape.setOutlineColor(sf::Color(241, 196, 15)); // Viền màu vàng
        } else {
            isHovered = false;
            shape.setFillColor(sf::Color(41, 128, 185));
            shape.setOutlineColor(sf::Color(52, 152, 219));
        }
    }

    bool isClicked(const sf::Vector2i& mousePos, bool mousePressed) {
        return isHovered && mousePressed;
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
        window.draw(text);
    }
};

class GuiMenu {
public:
    // Khởi chạy cửa sổ Menu GUI tương tác
    static void runGUIMenu(StationList& stationList, TrainList& trainList, TicketList& ticketList);
};
