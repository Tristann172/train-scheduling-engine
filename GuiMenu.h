#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

class StationList;
class TrainList;
class TicketList;

struct GuiButton {
    sf::RectangleShape shape;
    sf::Text text;
    int actionID;
    bool isHovered;

    GuiButton(float x, float y, float width, float height, const std::string& label, sf::Font& font, int id) 
        : text(font, label, 12), actionID(id), isHovered(false) 
    {
        shape.setPosition(sf::Vector2f(x, y));
        shape.setSize(sf::Vector2f(width, height));
        shape.setFillColor(sf::Color(41, 128, 185));
        shape.setOutlineThickness(1.5f);
        shape.setOutlineColor(sf::Color(52, 152, 219));

        text.setFillColor(sf::Color::White);
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setPosition(sf::Vector2f(
            x + (width - textBounds.size.x) / 2.0f - textBounds.position.x,
            y + (height - textBounds.size.y) / 2.0f - textBounds.position.y
        ));
    }

    void updateHover(const sf::Vector2i& mousePos) {
        if (shape.getGlobalBounds().contains(sf::Vector2f(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)))) {
            isHovered = true;
            shape.setFillColor(sf::Color(52, 152, 219));
            shape.setOutlineColor(sf::Color(241, 196, 15));
        } else {
            isHovered = false;
            shape.setFillColor(sf::Color(41, 128, 185));
            shape.setOutlineColor(sf::Color(52, 152, 219));
        }
    }

    bool isClicked(const sf::Vector2i& mousePos, bool mousePressed) const {
        return isHovered && mousePressed;
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
        window.draw(text);
    }
};

class GuiMenu {
public:
    static void runGUIMenu(StationList& stationList, TrainList& trainList, TicketList& ticketList);
    
    // Cửa sổ Pop-up GUI nhập liệu văn bản trực tiếp bằng bàn phím
    static bool showInputDialog(const std::string& title, const std::string& promptLabel, std::string& outputValue, sf::Font& font);
};
