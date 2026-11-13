#include "UiKit.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

using namespace std;

namespace Ui {

sf::String toSf(const string& utf8) {
    return sf::String::fromUtf8(utf8.begin(), utf8.end());
}

// Mã hóa một ký tự Unicode thành UTF-8 và nối vào chuỗi.
static void appendUtf8(string& out, char32_t code) {
    if (code < 0x80) {
        out.push_back(static_cast<char>(code));
    } else if (code < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (code >> 6)));
        out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
    } else if (code < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (code >> 12)));
        out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (code >> 18)));
        out.push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
    }
}

// Xóa ký tự cuối cùng theo đúng ranh giới UTF-8.
static void popUtf8(string& value) {
    while (!value.empty() && (static_cast<unsigned char>(value.back()) & 0xC0) == 0x80) {
        value.pop_back();
    }
    if (!value.empty()) value.pop_back();
}

Canvas::Canvas()
    : renderTarget(nullptr), hasArrowFont(false), viewScale(1.0f), originX(0.0f), originY(0.0f), frameDt(0.0f),
      totalTime(0.0f), mousePixel(-1, -1), mouseLogical(-1.0f, -1.0f), leftDown(false),
      leftPressed2(false), wheelDelta(0.0f), inputEnabled(true), handCursor(false), focusId(0),
      backspaces(0), enterPressed(false), escapePressed(false), tabPressed(false),
      spacePressed(false), leftPressed(false), rightPressed(false), tipActive(false),
      tipX(0.0f), tipY(0.0f) {
    for (int i = 0; i < ANIM_SLOTS; ++i) slots[i] = {0, 0.0f};
}

bool Canvas::loadFonts() {
    const char* paths[FONT_COUNT] = {
        "assets/fonts/Barlow-Regular.ttf",
        "assets/fonts/Barlow-SemiBold.ttf",
        "assets/fonts/BarlowCondensed-SemiBold.ttf",
        "assets/fonts/BarlowCondensed-Bold.ttf"
    };
    bool allLoaded = true;
    for (int i = 0; i < FONT_COUNT; ++i) {
        if (!fonts[i].openFromFile(paths[i]) && !fonts[i].openFromFile("assets/fonts/arial.ttf")) {
            allLoaded = false;
        }
    }
    hasArrowFont = arrowFont.openFromFile("assets/fonts/arial.ttf");
    return allLoaded;
}

void Canvas::feed(const sf::Event& event) {
    if (const auto* press = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (press->button == sf::Mouse::Button::Left) {
            leftDown = true;
            leftPressed2 = true;
        }
    } else if (const auto* release = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (release->button == sf::Mouse::Button::Left) leftDown = false;
    } else if (const auto* wheelEvent = event.getIf<sf::Event::MouseWheelScrolled>()) {
        wheelDelta += wheelEvent->delta;
    } else if (const auto* entered = event.getIf<sf::Event::TextEntered>()) {
        const char32_t code = entered->unicode;
        if (code == 8) ++backspaces;
        else if (code >= 32 && code != 127) {
            if (focusId != 0) appendUtf8(typedUtf8, code);
        }
    } else if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Enter) enterPressed = true;
        else if (key->code == sf::Keyboard::Key::Escape) escapePressed = true;
        else if (key->code == sf::Keyboard::Key::Tab) tabPressed = true;
        else if (key->code == sf::Keyboard::Key::Space) spacePressed = true;
        else if (key->code == sf::Keyboard::Key::Left) leftPressed = true;
        else if (key->code == sf::Keyboard::Key::Right) rightPressed = true;
    }
}

void Canvas::setMouse(sf::Vector2i pixel) {
    mousePixel = pixel;
}

void Canvas::beginFrame(sf::RenderTarget& target, float dt) {
    renderTarget = &target;
    frameDt = dt;
    totalTime += dt;
    const sf::Vector2u size = target.getSize();
    viewScale = min(size.x / DESIGN_W, size.y / DESIGN_H);
    originX = floor((size.x - DESIGN_W * viewScale) / 2.0f);
    originY = floor((size.y - DESIGN_H * viewScale) / 2.0f);
    mouseLogical = {(mousePixel.x - originX) / viewScale, (mousePixel.y - originY) / viewScale};
    handCursor = false;
    tipActive = false;
    clearScissor();
}

void Canvas::endFrame() {
    if (tipActive) {
        const float width = textWidth(tipText, FontRegular, 13.0f) + 16.0f;
        const float left = min(DESIGN_W - width - 4.0f, max(4.0f, tipX - width / 2.0f));
        rect(left, tipY - 28.0f, width, 24.0f, Palette::Ink);
        text(left + 8.0f, tipY - 16.0f, tipText, FontRegular, 13.0f, Palette::White);
    }
    leftPressed2 = false;
    wheelDelta = 0.0f;
    typedUtf8.clear();
    backspaces = 0;
    enterPressed = false;
    escapePressed = false;
    tabPressed = false;
    spacePressed = false;
    leftPressed = false;
    rightPressed = false;
}

sf::Vector2f Canvas::toPx(float x, float y) const {
    return {originX + x * viewScale, originY + y * viewScale};
}

void Canvas::setScissor(float x, float y, float w, float h) {
    const sf::Vector2u size = renderTarget->getSize();
    sf::View view(sf::FloatRect({0.0f, 0.0f}, {static_cast<float>(size.x), static_cast<float>(size.y)}));
    const sf::Vector2f topLeft = toPx(x, y);
    view.setScissor(sf::FloatRect({topLeft.x / size.x, topLeft.y / size.y},
                                  {w * viewScale / size.x, h * viewScale / size.y}));
    renderTarget->setView(view);
}

void Canvas::clearScissor() {
    const sf::Vector2u size = renderTarget->getSize();
    renderTarget->setView(sf::View(sf::FloatRect({0.0f, 0.0f},
                                                 {static_cast<float>(size.x), static_cast<float>(size.y)})));
}

void Canvas::rect(float x, float y, float w, float h, sf::Color color) {
    if (w <= 0.0f || h <= 0.0f) return;
    sf::RectangleShape shape({w * viewScale, h * viewScale});
    shape.setPosition(toPx(x, y));
    shape.setFillColor(color);
    renderTarget->draw(shape);
}

void Canvas::frame(float x, float y, float w, float h, float thick, sf::Color color) {
    rect(x, y, w, thick, color);
    rect(x, y + h - thick, w, thick, color);
    rect(x, y + thick, thick, h - 2.0f * thick, color);
    rect(x + w - thick, y + thick, thick, h - 2.0f * thick, color);
}

void Canvas::line(float x1, float y1, float x2, float y2, float thick, sf::Color color) {
    const float dx = x2 - x1;
    const float dy = y2 - y1;
    const float length = sqrt(dx * dx + dy * dy);
    if (length < 0.001f) return;
    sf::RectangleShape shape({length * viewScale, thick * viewScale});
    shape.setOrigin({0.0f, thick * viewScale / 2.0f});
    shape.setPosition(toPx(x1, y1));
    shape.setRotation(sf::radians(atan2(dy, dx)));
    shape.setFillColor(color);
    renderTarget->draw(shape);
}

void Canvas::circle(float cx, float cy, float r, sf::Color fill, float outline, sf::Color outlineColor) {
    const float radius = r * viewScale;
    sf::CircleShape shape(radius, 32);
    shape.setOrigin({radius, radius});
    shape.setPosition(toPx(cx, cy));
    shape.setFillColor(fill);
    if (outline > 0.0f) {
        shape.setOutlineThickness(-outline * viewScale);
        shape.setOutlineColor(outlineColor);
    }
    renderTarget->draw(shape);
}

void Canvas::hazard(float x, float y, float w, float h) {
    const float stripe = 5.0f;
    int index = 0;
    for (float left = x; left < x + w; left += stripe, ++index) {
        rect(left, y, min(stripe, x + w - left), h, index % 2 == 0 ? Palette::Hazard : Palette::WaitRed);
    }
}

// Vẽ chuỗi theo từng đoạn: đoạn mũi tên dùng font dự phòng. Trả về bề rộng theo điểm ảnh.
float Canvas::drawRuns(float penX, float penY, const string& value, FontId font, unsigned int px,
                       sf::Color color, bool draw) {
    const sf::String all = toSf(value);
    const auto isArrow = [](char32_t code) { return code >= 0x2190 && code <= 0x21FF; };
    float pen = 0.0f;
    size_t start = 0;
    while (start < all.getSize()) {
        const bool arrow = isArrow(all[start]);
        size_t end = start;
        while (end < all.getSize() && isArrow(all[end]) == arrow) ++end;
        sf::Text run((arrow && hasArrowFont) ? arrowFont : fonts[font], all.substring(start, end - start), px);
        const float advance = run.findCharacterPos(end - start).x;
        if (draw) {
            run.setFillColor(color);
            run.setPosition({round(penX + pen), round(penY)});
            renderTarget->draw(run);
        }
        pen += advance;
        start = end;
    }
    return pen;
}

float Canvas::text(float x, float ymid, const string& value, FontId font, float size,
                   sf::Color color, Align align) {
    if (value.empty()) return 0.0f;
    const unsigned int px = static_cast<unsigned int>(max(6.0f, round(size * viewScale)));
    const float width = drawRuns(0.0f, 0.0f, value, font, px, color, false);
    sf::Vector2f position = toPx(x, ymid);
    if (align == Align::Center) position.x -= width / 2.0f;
    else if (align == Align::Right) position.x -= width;
    // SFML đặt đường chân chữ tại y + cỡ chữ; Barlow có chữ hoa cao khoảng 0.7 em.
    position.y -= 0.65f * px;
    drawRuns(position.x, position.y, value, font, px, color, true);
    return width / viewScale;
}

float Canvas::textWidth(const string& value, FontId font, float size) {
    if (value.empty()) return 0.0f;
    const unsigned int px = static_cast<unsigned int>(max(6.0f, round(size * viewScale)));
    return drawRuns(0.0f, 0.0f, value, font, px, sf::Color::White, false) / viewScale;
}

void Canvas::textFit(float x, float ymid, const string& value, FontId font, float size,
                     sf::Color color, float maxWidth) {
    if (textWidth(value, font, size) <= maxWidth) {
        text(x, ymid, value, font, size, color);
        return;
    }
    string cut = value;
    while (!cut.empty() && textWidth(cut + "…", font, size) > maxWidth) popUtf8(cut);
    text(x, ymid, cut + "…", font, size, color);
}

bool Canvas::hovered(float x, float y, float w, float h) const {
    return inputEnabled && mouseLogical.x >= x && mouseLogical.x < x + w &&
           mouseLogical.y >= y && mouseLogical.y < y + h;
}

bool Canvas::clicked(float x, float y, float w, float h) const {
    return leftPressed2 && hovered(x, y, w, h);
}

bool Canvas::pressedAt(float x, float y, float w, float h) const {
    return leftDown && hovered(x, y, w, h);
}

float Canvas::anim(int id, float goal, float rate) {
    int index = (id & 0x7FFFFFFF) % ANIM_SLOTS;
    for (int probe = 0; probe < ANIM_SLOTS; ++probe) {
        AnimSlot& slot = slots[(index + probe) % ANIM_SLOTS];
        if (slot.id == id || slot.id == 0) {
            slot.id = id;
            slot.value += (goal - slot.value) * min(1.0f, frameDt * rate);
            return slot.value;
        }
    }
    return goal;
}

bool Canvas::button(int id, float x, float y, float w, float h, const string& label,
                    ButtonStyle style, bool selected) {
    const bool over = hovered(x, y, w, h);
    const bool down = over && leftDown;
    const float amount = anim(id, over ? 1.0f : 0.0f);
    if (over) handCursor = true;

    sf::Color fill = sf::Color::Transparent;
    sf::Color border = sf::Color::Transparent;
    sf::Color ink = Palette::Ink;
    FontId font = FontRegular;

    switch (style) {
    case ButtonStyle::Solid:
        fill = Palette::Ink; ink = Palette::White; font = FontSemi;
        break;
    case ButtonStyle::Accent:
        fill = Palette::Blue; ink = Palette::White; font = FontSemi;
        break;
    case ButtonStyle::Outline:
        border = Palette::Stone;
        break;
    case ButtonStyle::OutlineInk:
        border = Palette::Ink;
        break;
    case ButtonStyle::Danger:
        border = Palette::Red; ink = Palette::Red; font = FontSemi;
        break;
    case ButtonStyle::Nav:
    case ButtonStyle::Segment:
        if (style == ButtonStyle::Segment) border = Palette::Ink;
        if (selected) { fill = Palette::Ink; ink = Palette::White; font = FontSemi; }
        break;
    }

    const bool flat = style == ButtonStyle::Nav;
    float offset = 0.0f;
    if (!flat && !selected) offset = down ? 1.0f : -2.0f * amount;
    else if (flat && down) offset = 1.0f;

    if (!flat && !selected && amount > 0.01f && !down) {
        sf::Color shadow = Palette::Stone;
        shadow.a = static_cast<uint8_t>(255.0f * amount);
        rect(x, y + h + offset, w, 3.0f, shadow);
    }
    if (flat && !selected && amount > 0.01f) {
        sf::Color tint = down ? Palette::Rule : Palette::Hover;
        tint.a = static_cast<uint8_t>(255.0f * amount);
        rect(x, y, w, h, tint);
        sf::Color bar = Palette::Ink;
        bar.a = static_cast<uint8_t>(255.0f * amount);
        rect(x, y + h - 3.0f, w, 3.0f, bar);
    }
    if (fill.a > 0) rect(x, y + offset, w, h, fill);
    if (border.a > 0) frame(x, y + offset, w, h, 1.0f, border);
    text(x + w / 2.0f, y + offset + h / 2.0f, label, font, 15.0f, ink, Align::Center);

    return over && leftPressed2;
}

bool Canvas::field(int id, float x, float y, float w, float h, string& value,
                   const string& placeholder, FontId font, float size, bool readOnly) {
    const bool over = hovered(x, y, w, h);
    if (leftPressed2 && inputEnabled) {
        if (over && !readOnly) focusId = id;
        else if (focusId == id) focusId = 0;
    }
    const bool active = focusId == id && !readOnly;
    if (active) {
        if (!typedUtf8.empty() && value.size() < 120) value += typedUtf8;
        for (int i = 0; i < backspaces; ++i) popUtf8(value);
    }

    rect(x, y, w, h, readOnly ? Palette::Ground : Palette::White);
    if (over && !readOnly) {
        frame(x - 2.0f, y - 2.0f, w + 4.0f, h + 4.0f, 2.0f, Palette::Rule);
        handCursor = false;
    }
    frame(x, y, w, h, active ? 2.0f : 1.0f, active ? Palette::Blue : Palette::Ink);

    const float mid = y + h / 2.0f;
    const float inner = w - 20.0f;
    if (value.empty() && !active) {
        text(x + 10.0f, mid, placeholder, FontRegular, 15.0f, Palette::Muted);
    } else {
        string visible = value;
        while (!visible.empty() && textWidth(visible, font, size) > inner) {
            string::size_type cut = 1;
            while (cut < visible.size() && (static_cast<unsigned char>(visible[cut]) & 0xC0) == 0x80) ++cut;
            visible.erase(0, cut);
        }
        const float width = text(x + 10.0f, mid, visible, font, size, Palette::Ink);
        if (active && static_cast<int>(totalTime * 2.0f) % 2 == 0) {
            rect(x + 11.0f + width, mid - size * 0.55f, 1.5f, size * 1.1f, Palette::Ink);
        }
    }
    return active;
}

void Canvas::tooltip(float x, float y, const string& value) {
    tipActive = true;
    tipX = x;
    tipY = y;
    tipText = value;
}

void Canvas::spriteBox(float x, float y, float k, float px, float py, float pw, float ph,
                       float w, bool flip, sf::Color fill, bool outlined) {
    const float left = flip ? x + w - (px + pw) * k : x + px * k;
    if (outlined) rect(left - 0.6f, y + py * k - 0.6f, pw * k + 1.2f, ph * k + 1.2f, Palette::Ink);
    rect(left, y + py * k, pw * k, ph * k, fill);
}

// Vẽ ảnh tàu nhỏ bằng khối hình: tàu nhanh, tàu thường hoặc tàu hàng.
void Canvas::trainSprite(float x, float y, float w, float h, int priority, bool flip) {
    (void)h;
    const float k = w / 120.0f;
    const sf::Color ink = Palette::Ink;
    const bool freight = priority >= 3;
    const sf::Color loco = priority == 1 ? Palette::Blue
        : (priority == 2 ? Palette::Green : sf::Color(224, 138, 30));
    const sf::Color coach = priority == 2 ? sf::Color(221, 239, 229) : Palette::White;
    const sf::Color glass = priority == 1 ? sf::Color(214, 231, 248) : sf::Color(221, 239, 229);

    for (int c = 0; c < 2; ++c) {
        const float cx = 2.0f + 36.0f * c;
        if (freight) {
            spriteBox(x, y, k, cx, 20.0f, 32.0f, 4.0f, w, flip, ink, false);
            spriteBox(x, y, k, cx + 2.0f, 7.0f, 28.0f, 13.0f, w, flip,
                      c == 0 ? Palette::Amber : sf::Color(91, 102, 112), true);
        } else {
            spriteBox(x, y, k, cx, 6.0f, 32.0f, 18.0f, w, flip, coach, true);
            spriteBox(x, y, k, cx + 0.8f, 18.0f, 30.4f, 3.5f, w, flip, loco, false);
            for (int window = 0; window < 3; ++window) {
                spriteBox(x, y, k, cx + 4.0f + 9.0f * window, 9.5f, 6.0f, 5.5f, w, flip, ink, false);
            }
        }
        spriteBox(x, y, k, cx + 32.0f, 17.0f, 4.0f, 3.0f, w, flip, ink, false);
    }

    if (freight) {
        spriteBox(x, y, k, 74.0f, 12.0f, 44.0f, 12.0f, w, flip, loco, true);
        spriteBox(x, y, k, 94.0f, 4.0f, 16.0f, 9.0f, w, flip, loco, true);
        spriteBox(x, y, k, 98.0f, 7.0f, 8.0f, 6.0f, w, flip, sf::Color(251, 233, 207), false);
    } else {
        // Đầu máy: thân chữ nhật cộng mũi dốc vẽ bằng đa giác lồi.
        const float bodyEnd = priority == 1 ? 98.0f : 110.0f;
        spriteBox(x, y, k, 74.0f, 6.0f, bodyEnd - 74.0f, 18.0f, w, flip, loco, true);
        sf::ConvexShape nose(priority == 1 ? 5 : 4);
        const float points[5][2] = {{bodyEnd, 6.0f}, {108.0f, 9.0f}, {115.0f, 16.0f}, {118.0f, 24.0f}, {bodyEnd, 24.0f}};
        const float boxy[4][2] = {{bodyEnd, 6.0f}, {118.0f, 12.0f}, {118.0f, 24.0f}, {bodyEnd, 24.0f}};
        const int count = priority == 1 ? 5 : 4;
        for (int i = 0; i < count; ++i) {
            const float px = priority == 1 ? points[i][0] : boxy[i][0];
            const float py = priority == 1 ? points[i][1] : boxy[i][1];
            nose.setPoint(static_cast<size_t>(i), toPx(flip ? x + w - px * k : x + px * k, y + py * k));
        }
        nose.setFillColor(loco);
        renderTarget->draw(nose);
        spriteBox(x, y, k, 79.0f, 9.5f, 6.0f, 5.5f, w, flip, glass, false);
        spriteBox(x, y, k, 88.0f, 9.5f, 6.0f, 5.5f, w, flip, glass, false);
    }

    const float wheels[6] = {9.0f, 27.0f, 45.0f, 63.0f, 82.0f, 106.0f};
    for (int i = 0; i < 6; ++i) {
        const float wx = flip ? x + w - wheels[i] * k : x + wheels[i] * k;
        circle(wx, y + 26.5f * k, 3.0f * k, ink);
    }
}

}
