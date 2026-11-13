#pragma once

#include <SFML/Graphics.hpp>
#include <string>

// Bộ công cụ giao diện dùng chung: mọi màn hình vẽ trên khung thiết kế 1360x768,
// rồi được nhân tỉ lệ theo kích thước cửa sổ để chữ luôn sắc nét.
namespace Ui {

const float DESIGN_W = 1360.0f;
const float DESIGN_H = 768.0f;

namespace Palette {
    const sf::Color Ground(244, 245, 244);      // #F4F5F4 - Nền màn hình
    const sf::Color White(255, 255, 255);
    const sf::Color Ink(20, 23, 26);            // #14171A - Chữ chính, đường kẻ đậm
    const sf::Color Body(58, 66, 73);           // #3A4249 - Chữ phụ đậm
    const sf::Color Muted(85, 94, 102);         // #555E66 - Chữ phụ
    const sf::Color Stone(138, 148, 156);       // #8A949C - Viền nút, lịch công bố
    const sf::Color Rule(205, 210, 213);        // #CDD2D5 - Đường kẻ
    const sf::Color RuleSoft(225, 228, 230);    // #E1E4E6 - Đường kẻ nhạt
    const sf::Color Hover(227, 230, 230);       // #E3E6E6 - Nền khi di chuột
    const sf::Color Blue(11, 87, 176);          // #0B57B0 - Tàu đang chạy
    const sf::Color Red(179, 38, 30);           // #B3261E - Cảnh báo dạng chữ
    const sf::Color WaitRed(217, 45, 32);       // #D92D20 - Đoạn chờ, sọc bảo trì
    const sf::Color Hazard(245, 197, 24);       // #F5C518 - Sọc vàng bảo trì
    const sf::Color Amber(180, 83, 9);          // #B45309 - Tàu đỗ ga
    const sf::Color Violet(109, 40, 217);       // #6D28D9 - Tàu tránh ở ray phụ
    const sf::Color Green(31, 107, 69);         // #1F6B45 - Đúng giờ
    const sf::Color Sea(220, 229, 235);         // #DCE5EB - Biển
    const sf::Color Neighbor(236, 237, 234);    // #ECEDEA - Nước láng giềng
    const sf::Color NeighborLine(196, 202, 206);// #C4CACE - Biên giới láng giềng
    const sf::Color Mid(211, 219, 224);         // #D3DBE0 - Ga có 3 đến 5 ray
}

enum FontId { FontRegular, FontSemi, FontCondSemi, FontCondBold, FONT_COUNT };
enum class Align { Left, Center, Right };
enum class ButtonStyle { Solid, Outline, OutlineInk, Accent, Danger, Nav, Segment };

// Chuỗi UTF-8 sang sf::String để hiển thị đúng tiếng Việt có dấu.
sf::String toSf(const std::string& utf8);

class Canvas {
public:
    Canvas();

    bool loadFonts();
    void feed(const sf::Event& event);
    void setMouse(sf::Vector2i pixel);
    void beginFrame(sf::RenderTarget& target, float dt);
    void endFrame();

    // Khóa/mở tương tác, dùng khi có hộp thoại phủ lên màn hình.
    void setInputEnabled(bool enabled) { inputEnabled = enabled; }

    sf::RenderTarget& target() { return *renderTarget; }
    float scale() const { return viewScale; }
    float deltaTime() const { return frameDt; }
    float clock() const { return totalTime; }
    sf::Vector2f toPx(float x, float y) const;
    sf::Vector2f mouse() const { return mouseLogical; }
    bool mouseDown() const { return leftDown; }
    float wheel() const { return inputEnabled ? wheelDelta : 0.0f; }
    bool wantsHand() const { return handCursor; }
    bool keyEnter() const { return enterPressed; }
    bool keyEscape() const { return escapePressed; }
    bool keyTab() const { return tabPressed; }
    bool keySpace() const { return spacePressed && focusId == 0 && inputEnabled; }
    bool keyLeft() const { return leftPressed && focusId == 0 && inputEnabled; }
    bool keyRight() const { return rightPressed && focusId == 0 && inputEnabled; }

    void setScissor(float x, float y, float w, float h);
    void clearScissor();

    void rect(float x, float y, float w, float h, sf::Color color);
    void frame(float x, float y, float w, float h, float thick, sf::Color color);
    void line(float x1, float y1, float x2, float y2, float thick, sf::Color color);
    void circle(float cx, float cy, float r, sf::Color fill, float outline = 0.0f,
                sf::Color outlineColor = sf::Color::Transparent);
    void hazard(float x, float y, float w, float h);

    // ymid là đường giữa dòng chữ. Trả về bề rộng đã vẽ theo đơn vị thiết kế.
    float text(float x, float ymid, const std::string& value, FontId font, float size,
               sf::Color color, Align align = Align::Left);
    float textWidth(const std::string& value, FontId font, float size);
    void textFit(float x, float ymid, const std::string& value, FontId font, float size,
                 sf::Color color, float maxWidth);

    bool hovered(float x, float y, float w, float h) const;
    bool clicked(float x, float y, float w, float h) const;
    bool pressedAt(float x, float y, float w, float h) const;
    float anim(int id, float goal, float rate = 14.0f);

    bool button(int id, float x, float y, float w, float h, const std::string& label,
                ButtonStyle style, bool selected = false);
    bool field(int id, float x, float y, float w, float h, std::string& value,
               const std::string& placeholder, FontId font = FontRegular, float size = 15.0f,
               bool readOnly = false);
    void focus(int id) { focusId = id; }
    int focused() const { return focusId; }
    void clearFocus() { focusId = 0; }
    void markHand() { if (inputEnabled) handCursor = true; }

    void tooltip(float x, float y, const std::string& value);
    void trainSprite(float x, float y, float w, float h, int priority, bool flip);

private:
    struct AnimSlot { int id; float value; };
    static const int ANIM_SLOTS = 509;

    sf::RenderTarget* renderTarget;
    sf::Font fonts[FONT_COUNT];
    sf::Font arrowFont;   // Barlow không có ký tự mũi tên nên dùng arial.ttf cho riêng các ký tự này
    bool hasArrowFont;
    AnimSlot slots[ANIM_SLOTS];

    float viewScale;
    float originX;
    float originY;
    float frameDt;
    float totalTime;
    sf::Vector2i mousePixel;
    sf::Vector2f mouseLogical;

    bool leftDown;
    bool leftPressed2;
    float wheelDelta;
    bool inputEnabled;
    bool handCursor;

    int focusId;
    std::string typedUtf8;
    int backspaces;
    bool enterPressed;
    bool escapePressed;
    bool tabPressed;
    bool spacePressed;
    bool leftPressed;
    bool rightPressed;

    bool tipActive;
    float tipX;
    float tipY;
    std::string tipText;

    float drawRuns(float penX, float penY, const std::string& value, FontId font, unsigned int px,
                   sf::Color color, bool draw);
    void spriteBox(float x, float y, float k, float px, float py, float pw, float ph,
                   float w, bool flip, sf::Color fill, bool outlined);
};

}
