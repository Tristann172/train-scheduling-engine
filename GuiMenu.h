#pragma once

#include <SFML/Graphics.hpp>
#include <string>

class StationList;
class TrainList;

// Bảng màu đồng bộ với GraphicalSimulationEngine (Dracula Dark Mode)
namespace GuiTheme {
    const sf::Color BgMain(18, 20, 29);          // #12141D - Nền tổng thể
    const sf::Color PanelBg(28, 32, 46);         // #1C202E - Nền khung nội dung
    const sf::Color CardBg(36, 41, 58);          // #24293A - Nền thẻ KPI
    const sf::Color Border(62, 70, 96);          // #3E4660 - Viền khung

    const sf::Color BtnMain(41, 47, 70);         // Màu nút nhóm chính
    const sf::Color BtnMainHover(56, 65, 96);    // Màu nút chính khi di chuột
    const sf::Color BtnMainActive(46, 134, 222); // Màu nút chính khi đang mở
    
    const sf::Color BtnSub(30, 35, 52);          // Màu nút chức năng con
    const sf::Color BtnSubHover(68, 78, 115);    // Màu nút con khi di chuột

    const sf::Color AccentGold(241, 250, 140);   // #F1FA8C - Vàng nhấn
    const sf::Color AccentCyan(0, 229, 255);     // #00E5FF - Xanh Cyan
    const sf::Color AccentGreen(80, 250, 123);   // #50FA7B - Xanh lá
    const sf::Color AccentRed(255, 85, 85);      // #FF5555 - Đỏ cảnh báo
    const sf::Color AccentPurple(189, 147, 249); // #BD93F9 - Tím Dracula
    const sf::Color TextLight(248, 248, 242);    // Chữ chính
    const sf::Color TextMuted(148, 163, 184);    // Chữ phụ
}

struct GuiButton {
    sf::RectangleShape shape;
    sf::RectangleShape accentBar; // Thanh chỉ báo màu bên trái nút
    sf::Text text;
    int actionID;
    bool isHovered;
    bool isSelected;
    bool isSubButton;

    // Constructor mặc định phục vụ cấp phát mảng tĩnh
    GuiButton() : text(*(new sf::Font()), "", 12), actionID(-1), isHovered(false), isSelected(false), isSubButton(false) {}

    void init(float x, float y, float width, float height, const std::string& label, sf::Font& font, int id, bool sub = false) {
        actionID = id;
        isHovered = false;
        isSelected = false;
        isSubButton = sub;

        shape.setPosition(sf::Vector2f(x, y));
        shape.setSize(sf::Vector2f(width, height));
        shape.setFillColor(isSubButton ? GuiTheme::BtnSub : GuiTheme::BtnMain);
        shape.setOutlineThickness(1.0f);
        shape.setOutlineColor(GuiTheme::Border);

        accentBar.setPosition(sf::Vector2f(x, y));
        accentBar.setSize(sf::Vector2f(4.0f, height));
        accentBar.setFillColor(isSubButton ? GuiTheme::AccentCyan : GuiTheme::AccentGold);

        text.setFont(font);
        text.setString(label);
        text.setCharacterSize(isSubButton ? 12 : 13);
        text.setFillColor(GuiTheme::TextLight);

        updatePosition(x, y);
    }

    void updatePosition(float x, float y) {
        shape.setPosition(sf::Vector2f(x, y));
        accentBar.setPosition(sf::Vector2f(x, y));
        sf::FloatRect textBounds = text.getLocalBounds();
        // Căn lề trái có khoảng đệm hiện đại thay vì căn giữa đơn điệu
        text.setPosition(sf::Vector2f(
            x + 14.0f,
            y + (shape.getSize().y - textBounds.size.y) / 2.0f - textBounds.position.y
        ));
    }

    void updateHover(const sf::Vector2i& mousePos) {
        sf::Vector2f mp(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
        isHovered = shape.getGlobalBounds().contains(mp);

        if (isHovered) {
            shape.setFillColor(isSubButton ? GuiTheme::BtnSubHover : GuiTheme::BtnMainHover);
            shape.setOutlineColor(isSubButton ? GuiTheme::AccentCyan : GuiTheme::AccentGold);
        } else {
            if (isSelected && !isSubButton) {
                shape.setFillColor(GuiTheme::BtnMainActive);
                shape.setOutlineColor(GuiTheme::AccentGold);
            } else {
                shape.setFillColor(isSubButton ? GuiTheme::BtnSub : GuiTheme::BtnMain);
                shape.setOutlineColor(GuiTheme::Border);
            }
        }
    }

    void setSelected(bool selected) {
        isSelected = selected;
        updateHover({-1, -1});
    }

    bool isClicked(bool mousePressed) const {
        return isHovered && mousePressed;
    }

    void draw(sf::RenderWindow& window) const {
        window.draw(shape);
        if (isSelected || isHovered) {
            window.draw(accentBar);
        }
        window.draw(text);
    }
};

// Cấu trúc Nhóm Menu dùng mảng tĩnh (Không dùng std::vector)
struct MenuGroup {
    static const int MAX_SUB_BTNS = 10;
    GuiButton mainCategoryBtn;
    GuiButton subButtons[MAX_SUB_BTNS];
    int subCount;
    bool isExpanded;

    MenuGroup() : subCount(0), isExpanded(false) {}

    void initGroup(float x, float y, float w, float h, const std::string& label, sf::Font& font, int categoryID) {
        mainCategoryBtn.init(x, y, w, h, label, font, categoryID, false);
        subCount = 0;
        isExpanded = false;
    }

    void addSubButton(float x, float y, float w, float h, const std::string& label, sf::Font& font, int actionID) {
        if (subCount < MAX_SUB_BTNS) {
            subButtons[subCount].init(x, y, w, h, label, font, actionID, true);
            subCount++;
        }
    }
};

class GuiMenu {
public:
    static void runGUIMenu(StationList& stationList, TrainList& trainList);
    static bool showInputDialog(const std::string& title, const std::string& promptLabel, std::string& outputValue, sf::Font& font);
    static void applyDelayUI(StationList& stationList, TrainList& trainList, sf::Font& font, std::string& outputLog);
};