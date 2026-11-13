#include "GuiMenu.h"

#include "CarriageNode.h"
#include "FileManager.h"
#include "StationList.h"
#include "StationNode.h"
#include "StopScheduleNode.h"
#include "TrainNode.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <stdexcept>

using namespace std;
using namespace Ui;

namespace {

string trim(const string& value) {
    string::size_type first = value.find_first_not_of(" \t");
    if (first == string::npos) return "";
    return value.substr(first, value.find_last_not_of(" \t") - first + 1);
}

bool parseInteger(const string& text, int& out) {
    const string value = trim(text);
    if (value.empty()) return false;
    try {
        string::size_type used = 0;
        out = stoi(value, &used);
        return used == value.size();
    } catch (const exception&) {
        return false;
    }
}

bool parseReal(const string& text, double& out) {
    const string value = trim(text);
    if (value.empty()) return false;
    try {
        string::size_type used = 0;
        out = stod(value, &used);
        return used == value.size();
    } catch (const exception&) {
        return false;
    }
}

// Đọc giờ dạng "09:20" hoặc "D+1 01:00"; số nguyên được hiểu là phút tuyệt đối.
bool parseTime(const string& text, int defaultDay, int& out) {
    string value = trim(text);
    if (value.empty()) return false;
    if (value.find(':') == string::npos) return parseInteger(value, out) && out >= 0;

    int day = defaultDay;
    if (value.size() > 2 && (value[0] == 'D' || value[0] == 'd') && value[1] == '+') {
        const string::size_type space = value.find(' ');
        if (space == string::npos || !parseInteger(value.substr(2, space - 2), day) || day < 0) return false;
        value = trim(value.substr(space + 1));
    }
    const string::size_type colon = value.find(':');
    int hour = 0;
    int minute = 0;
    if (colon == string::npos || !parseInteger(value.substr(0, colon), hour) ||
        !parseInteger(value.substr(colon + 1), minute)) return false;
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return false;
    out = day * 1440 + hour * 60 + minute;
    return true;
}

string groupDigits(int value) {
    string digits = to_string(value);
    for (int i = static_cast<int>(digits.size()) - 3; i > 0; i -= 3) digits.insert(static_cast<size_t>(i), ".");
    return digits;
}

string priorityName(int priority) {
    if (priority == 1) return "P1 · Tàu khách nhanh";
    if (priority == 2) return "P2 · Tàu khách thường";
    return "P3 · Tàu hàng";
}

const int FIELD_BASE = 9100;

}

RailwayApp::RailwayApp(StationList& stationList, TrainList& trainList)
    : stations(stationList), trains(trainList), screen(AppScreen::Stations), running(true),
      noticeIsError(false), noticeTime(0.0f), stationScroll(0), trainScroll(0), windowScroll(0),
      formKind(FormKind::None), formChoiceA(0), formChoiceB(0), formTrack(1), formDayA(0), formDayB(0),
      formStation(0), formCarriageNo(0), confirmAction(ConfirmAction::None), confirmWindow(-1) {}

void RailwayApp::init() {
    ui.loadFonts();
    map.load("data/map_data.txt");
    FileManager::loadTrains("data/timetable_base.txt", baseline);
    trains.resolveConflictAndScheduleSiding(&stations);
    refresh();
    simulation.resetClock(model);
    if (trains.getHead()) selectedTrain = trains.getHead()->data->getTrainCode();
}

void RailwayApp::refresh() {
    model.rebuild(stations, trains, baseline, map);
}

void RailwayApp::setNotice(const string& text, bool isError) {
    notice = text;
    noticeIsError = isError;
    noticeTime = 5.0f;
}

bool RailwayApp::saveAll() {
    const bool ok = FileManager::saveTrains("data/trains.txt", trains) &&
                    FileManager::saveStations("data/stations.txt", stations);
    if (!ok) setNotice("Không ghi được dữ liệu ra file.", true);
    return ok;
}

// ==================== THAO TÁC DỮ LIỆU ====================
void RailwayApp::scanConflicts() {
    trains.resolveConflictAndScheduleSiding(&stations);
    saveAll();
    refresh();
    if (trains.getLogCount() == 0) setNotice("Toàn tuyến không phát sinh xung đột mới.");
    else setNotice("Đã xử lý " + to_string(trains.getLogCount()) + " sự kiện, xem Nhật ký điều phối.");
}

void RailwayApp::reloadData() {
    stations.clear();
    trains.clear();
    FileManager::loadStations("data/stations.txt", stations);
    FileManager::loadTrains("data/trains.txt", trains);
    trains.resolveConflictAndScheduleSiding(&stations);
    setNotice("Đã nạp lại dữ liệu ga, tàu và lịch bảo trì.");
    refresh();
}

// Đóng hoặc mở tài nguyên trong bộ nhớ, sau đó lưu trực tiếp vào trains.txt.
bool RailwayApp::changeMaintenance(bool opening, const string& resource, int first, int second,
                                   int start, int end, string& error) {
    StationNode* firstStation = stations.findStationByID(first);
    StationNode* secondStation = resource == "BLOCK" ? stations.findStationByID(second) : nullptr;
    bool valid = firstStation != nullptr;
    if (resource == "BLOCK") {
        valid = valid && secondStation && first != second &&
                firstStation->getKmMarker() != secondStation->getKmMarker();
        if (valid) {
            const double low = min(firstStation->getKmMarker(), secondStation->getKmMarker());
            const double high = max(firstStation->getKmMarker(), secondStation->getKmMarker());
            for (LinkedList<StationNode*>::Node* st = stations.getHead(); st; st = st->next) {
                if (st->data->getKmMarker() > low && st->data->getKmMarker() < high) valid = false;
            }
        }
    } else if (resource == "TRACK") {
        valid = valid && second >= 1 && second <= firstStation->getMaxTracks();
    }
    if (!valid) {
        error = resource == "BLOCK" ? "Khu gian phải nối hai ga liền kề và có thật."
                                    : "Ga hoặc số ray không tồn tại.";
        return false;
    }
    if (start < 0 || end <= start) {
        error = "Giờ kết thúc phải sau giờ bắt đầu.";
        return false;
    }

    const bool changed = opening ? trains.removeMaintenance(resource, first, second, start, end)
                                 : trains.addMaintenance(resource, first, second, start, end);
    if (!changed) {
        error = "Không tìm thấy lịch bảo trì này.";
        return false;
    }

    const auto rollback = [&]() {
        if (opening) trains.addMaintenance(resource, first, second, start, end);
        else trains.removeMaintenance(resource, first, second, start, end);
        trains.resolveConflictAndScheduleSiding(&stations);
    };

    trains.resolveConflictAndScheduleSiding(&stations);
    if (!FileManager::saveTrains("data/trains.txt", trains)) {
        rollback();
        error = "Không ghi được trains.txt, đã khôi phục trạng thái.";
        return false;
    }
    refresh();
    return true;
}

// ==================== KHUNG CHUNG ====================
void RailwayApp::drawHeader() {
    ui.text(32.0f, 44.0f, "Điều hành Đường sắt Tuyến đơn", FontCondBold, 28.0f, Palette::Ink);

    const char* const labels[5] = {"Nhà ga", "Tàu & lịch trình", "Bảo trì & sự cố", "Bản đồ mô phỏng", "Lưu & thoát"};
    float widths[5];
    float total = 16.0f;
    for (int i = 0; i < 5; ++i) {
        widths[i] = ui.textWidth(labels[i], i == 0 ? FontSemi : FontRegular, 15.0f) + 28.0f;
        total += widths[i];
    }
    float x = 1328.0f - total;
    for (int i = 0; i < 5; ++i) {
        const bool current =
            (i == 0 && screen == AppScreen::Stations) ||
            (i == 1 && screen == AppScreen::Trains) ||
            (i == 2 && screen == AppScreen::Maintenance) ||
            (i == 3 && (screen == AppScreen::Map || screen == AppScreen::Chart));
        if (ui.button(100 + i, x, 20.0f, widths[i], 48.0f, labels[i], ButtonStyle::Nav, current)) {
            if (i == 0) screen = AppScreen::Stations;
            else if (i == 1) screen = AppScreen::Trains;
            else if (i == 2) screen = AppScreen::Maintenance;
            else if (i == 3) screen = AppScreen::Map;
            else running = false;
        }
        x += widths[i] + 4.0f;
    }
    ui.rect(32.0f, 68.0f, 1296.0f, 2.0f, Palette::Ink);
}

// In nhật ký: đổi mã ga trong thẻ [..] thành tên ga để dễ đọc.
void RailwayApp::drawLog(float x, float y, float width) {
    float cursor = x + ui.text(x, y, "Nhật ký điều phối", FontCondSemi, 18.0f, Palette::Ink) + 24.0f;
    float row = y;
    const int count = trains.getLogCount();
    if (count == 0) {
        ui.text(cursor, row, "Chưa có sự kiện mới", FontRegular, 14.0f, Palette::Muted);
        return;
    }
    const auto withNames = [&](string value) {
        string out;
        for (string::size_type i = 0; i < value.size();) {
            if (isdigit(static_cast<unsigned char>(value[i])) && (i == 0 || value[i - 1] == ' ' || value[i - 1] == '>')) {
                string::size_type end = i;
                while (end < value.size() && isdigit(static_cast<unsigned char>(value[end]))) ++end;
                const SimStation* st = model.findStation(atoi(value.substr(i, end - i).c_str()));
                out += st ? st->name : value.substr(i, end - i);
                i = end;
            } else {
                out.push_back(value[i++]);
            }
        }
        string::size_type arrow = out.find("->");
        if (arrow != string::npos) out.replace(arrow, 2, " → ");
        if (out.rfind("Ga ", 0) == 0) out.erase(0, 3);
        return out;
    };
    for (int i = count - 1; i >= max(0, count - 4); --i) {
        const string raw = trains.getLog(i);
        const string::size_type close = raw.find(']');
        const string tag = close == string::npos ? "" : withNames(raw.substr(1, close - 1));
        const string body = close == string::npos ? raw : trim(raw.substr(close + 1));
        const float tagW = ui.textWidth(tag, FontRegular, 14.0f);
        const float bodyW = ui.textWidth(body, FontRegular, 14.0f);
        if (cursor + tagW + bodyW + 8.0f > x + width) {
            cursor = x;
            row += 20.0f;
            if (row > y + 40.0f) return;
        }
        ui.text(cursor, row, tag, FontRegular, 14.0f, Palette::Muted);
        ui.text(cursor + tagW + 8.0f, row, body, FontRegular, 14.0f, Palette::Ink);
        cursor += tagW + bodyW + 32.0f;
    }
}

void RailwayApp::drawNotice() {
    if (noticeTime <= 0.0f) return;
    noticeTime -= ui.deltaTime();
    const float width = min(760.0f, ui.textWidth(notice, FontSemi, 15.0f) + 32.0f);
    const float x = (DESIGN_W - width) / 2.0f;
    ui.rect(x, 716.0f, width, 36.0f, noticeIsError ? Palette::Red : Palette::Ink);
    ui.textFit(x + 16.0f, 734.0f, notice, FontSemi, 15.0f, Palette::White, width - 32.0f);
}

// ==================== MÀN NHÀ GA ====================
void RailwayApp::drawStations() {
    const float rowY = 80.0f;
    const double lastKm = model.stationCount() ? model.station(model.stationCount() - 1).km : 0.0;
    const string summary = to_string(model.stationCount()) + " ga · " + groupDigits(static_cast<int>(lastKm)) + " km";
    const float summaryW = ui.text(32.0f, rowY + 22.0f, summary, FontCondSemi, 22.0f, Palette::Ink);

    float x = 32.0f + summaryW + 20.0f;
    x += ui.text(x, rowY + 22.0f, "Tìm ID", FontRegular, 15.0f, Palette::Ink) + 8.0f;
    ui.field(200, x, rowY, 110.0f, 44.0f, stationSearch, "VD: 104");
    x += 122.0f;
    if (ui.button(201, x, rowY, 92.0f, 44.0f, "Thêm ga", ButtonStyle::Solid)) openForm(FormKind::AddStation);

    float right = 1328.0f;
    right -= 124.0f;
    if (ui.button(205, right, rowY, 124.0f, 44.0f, "Mở mô phỏng", ButtonStyle::Accent)) screen = AppScreen::Map;
    right -= 124.0f;
    if (ui.button(204, right, rowY, 112.0f, 44.0f, "Nạp lại từ file", ButtonStyle::Outline)) reloadData();
    right -= 130.0f;
    if (ui.button(202, right, rowY, 118.0f, 44.0f, "Quét xung đột", ButtonStyle::Outline)) scanConflicts();

    simulation.drawStaticMap(ui, map, model, 32.0f, 134.0f);

    const float tableX = 618.0f;
    const float tableW = 710.0f;
    const float tableY = 134.0f;
    const float colId = tableX + 12.0f;
    const float colName = colId + 56.0f;
    const float colKm = colName + 160.0f;
    const float colRay = colKm + 92.0f;
    const float colNote = colRay + 100.0f;
    const float actionX = tableX + tableW - 118.0f;
    const float noteW = actionX - 8.0f - colNote;

    int indexes[SIM_MAX_STATIONS];
    int matches = 0;
    for (int i = 0; i < model.stationCount(); ++i) {
        if (stationSearch.empty() || to_string(model.station(i).id).find(trim(stationSearch)) != string::npos) {
            indexes[matches++] = i;
        }
    }
    const int visible = 18;
    const float rowH = 26.0f;
    if (ui.hovered(tableX, tableY, tableW, 30.0f + visible * rowH)) stationScroll -= static_cast<int>(ui.wheel());
    stationScroll = max(0, min(stationScroll, max(0, matches - visible)));

    ui.rect(tableX, tableY, tableW, 34.0f + visible * rowH, Palette::White);
    ui.rect(tableX, tableY, tableW, 2.0f, Palette::Ink);
    const float headY = tableY + 17.0f;
    ui.text(colId, headY, "ID", FontCondSemi, 16.0f, Palette::Ink);
    ui.text(colName, headY, "Tên ga", FontCondSemi, 16.0f, Palette::Ink);
    ui.text(colKm, headY, "Lý trình", FontCondSemi, 16.0f, Palette::Ink);
    ui.text(colRay, headY, "Số ray", FontCondSemi, 16.0f, Palette::Ink);
    ui.text(colNote, headY, "Ghi chú", FontCondSemi, 16.0f, Palette::Ink);
    ui.text(actionX, headY, "Thao tác", FontCondSemi, 16.0f, Palette::Ink);
    ui.rect(tableX, tableY + 32.0f, tableW, 1.0f, Palette::Ink);

    for (int r = 0; r < visible && r + stationScroll < matches; ++r) {
        const SimStation& st = model.station(indexes[r + stationScroll]);
        const float top = tableY + 33.0f + r * rowH;
        const float mid = top + rowH / 2.0f;
        ui.text(colId, mid, to_string(st.id), FontRegular, 14.0f, Palette::Muted);
        ui.text(colName, mid, st.name, FontSemi, 15.0f, Palette::Ink);
        ui.text(colKm, mid, groupDigits(static_cast<int>(st.km)) + " km", FontRegular, 14.0f, Palette::Ink);
        const int ticks = min(st.tracks, 16);
        for (int k = 0; k < ticks; ++k) ui.rect(colRay + k * 6.0f, mid - 7.0f, 4.0f, 14.0f, Palette::Ink);
        ui.text(colRay + ticks * 6.0f + 6.0f, mid, to_string(st.tracks), FontSemi, 14.0f, Palette::Ink);

        string note = st.tracks == 1 ? "Ga xép, không tránh được tàu" : to_string(st.tracks - 1) + " ray tránh";
        bool hot = false;
        for (int w = 0; w < model.windowCount(); ++w) {
            const SimWindow& window = model.window(w);
            if (window.type == 0 && window.first == st.id) {
                note = "Bảo trì đến " + formatMinute(window.end);
                hot = true;
            } else if (window.type == 1 && window.first == st.id) {
                note = "Khu gian sau bảo trì đến " + formatMinute(window.end);
                hot = true;
            } else if (window.type == 2 && window.first == st.id) {
                note = "Ray " + to_string(window.second) + " bảo trì đến " + formatMinute(window.end);
                hot = true;
            }
        }
        ui.textFit(colNote, mid, note, hot ? FontSemi : FontRegular, 14.0f, hot ? Palette::Red : Palette::Muted, noteW);
        bool actionClicked = false;
        if (ui.button(6000 + r * 2, actionX, top + 2.0f, 52.0f, 22.0f, "Sửa", ButtonStyle::Outline)) {
            openEditStation(st.id);
            actionClicked = true;
        }
        if (ui.button(6001 + r * 2, actionX + 58.0f, top + 2.0f, 52.0f, 22.0f, "Xóa", ButtonStyle::Danger)) {
            formStation = st.id;
            confirmAction = ConfirmAction::RemoveStation;
            confirmText = "Xóa ga " + st.name + "? Ga không được có tàu dừng hoặc lịch bảo trì.";
            openForm(FormKind::Confirm);
            actionClicked = true;
        }
        if (!actionClicked && ui.clicked(tableX, top, tableW, rowH)) openEditStation(st.id);
        ui.rect(tableX, top + rowH - 1.0f, tableW, 1.0f, Palette::RuleSoft);
    }
    ui.rect(tableX, tableY + 33.0f + visible * rowH, tableW, 2.0f, Palette::Ink);

    drawLog(tableX, tableY + 33.0f + visible * rowH + 26.0f, tableW);
}

// ==================== MÀN TÀU & LỊCH TRÌNH ====================
void RailwayApp::drawTrains() {
    const float rowY = 80.0f;
    const float countW = ui.text(32.0f, rowY + 22.0f, to_string(model.trainCount()) + " đoàn tàu", FontCondSemi, 22.0f, Palette::Ink);
    float x = 32.0f + countW + 20.0f;
    x += ui.text(x, rowY + 22.0f, "Tìm mã", FontRegular, 15.0f, Palette::Ink) + 8.0f;
    ui.field(300, x, rowY, 100.0f, 44.0f, trainSearch, "VD: SE1");
    x += 112.0f;
    if (ui.button(301, x, rowY, 92.0f, 44.0f, "Thêm tàu", ButtonStyle::Solid)) openForm(FormKind::AddTrain);

    const float listX = 32.0f;
    const float listW = 330.0f;
    const float listY = 134.0f;
    const float rowH = 68.0f;
    int indexes[SIM_MAX_TRAINS];
    int matches = 0;
    for (int i = 0; i < model.trainCount(); ++i) {
        if (trainSearch.empty() || model.train(i).code.find(trim(trainSearch)) != string::npos) indexes[matches++] = i;
    }
    const int visible = 8;
    if (ui.hovered(listX, listY, listW, visible * rowH)) trainScroll -= static_cast<int>(ui.wheel());
    trainScroll = max(0, min(trainScroll, max(0, matches - visible)));

    ui.rect(listX, listY, listW, 2.0f, Palette::Ink);
    for (int r = 0; r < visible && r + trainScroll < matches; ++r) {
        const SimTrain& train = model.train(indexes[r + trainScroll]);
        const float top = listY + 2.0f + r * rowH;
        const bool selected = train.code == selectedTrain;
        const bool over = ui.hovered(listX, top, listW, rowH);
        const float amount = ui.anim(3000 + indexes[r + trainScroll], over ? 1.0f : 0.0f);
        sf::Color tint = Palette::Hover;
        tint.a = static_cast<uint8_t>(255.0f * amount);
        ui.rect(listX, top, listW, rowH, selected ? Palette::White : tint);
        if (selected) ui.frame(listX, top, listW, rowH, 2.0f, Palette::Ink);
        if (over) ui.markHand();
        if (ui.clicked(listX, top, listW, rowH)) selectedTrain = train.code;

        ui.trainSprite(listX + 12.0f, top + 18.0f, 96.0f, 26.0f, train.priority, train.direction != 1);
        ui.text(listX + 122.0f, top + 21.0f, train.code, FontCondBold, 24.0f, Palette::Ink);
        const string route = train.stopCount > 1
            ? model.nameOf(train.stops[0].stationID) + " → " + model.nameOf(train.stops[train.stopCount - 1].stationID)
            : "Chưa có lịch";
        ui.textFit(listX + 122.0f, top + 47.0f, route, FontRegular, 14.0f, Palette::Muted, listW - 134.0f);
        if (!selected) ui.rect(listX, top + rowH - 1.0f, listW, 1.0f, Palette::Rule);
    }

    const SimTrain* current = nullptr;
    int currentIndex = -1;
    for (int i = 0; i < model.trainCount(); ++i) {
        if (model.train(i).code == selectedTrain) {
            current = &model.train(i);
            currentIndex = i;
        }
    }
    (void)currentIndex;
    const float detailX = 386.0f;
    const float detailW = 942.0f;
    if (!current) {
        ui.text(detailX, 160.0f, "Chọn một đoàn tàu ở danh sách bên trái.", FontRegular, 15.0f, Palette::Muted);
        return;
    }

    const TrainNode* source = trains.findTrainByCode(current->code);
    float right = 1328.0f;
    right -= 116.0f;
    if (ui.button(304, right, rowY, 116.0f, 44.0f, "Bãi bỏ chuyến", ButtonStyle::Danger)) {
        confirmAction = ConfirmAction::RemoveTrain;
        confirmText = "Bãi bỏ chuyến " + current->code + "? Toàn bộ lịch của tàu này sẽ bị xóa.";
        openForm(FormKind::Confirm);
    }
    right -= 148.0f;
    if (ui.button(303, right, rowY, 136.0f, 44.0f, "Thêm điểm dừng", ButtonStyle::Outline)) openForm(FormKind::AddStop);
    right -= 100.0f;
    if (ui.button(302, right, rowY, 88.0f, 44.0f, "Gán toa", ButtonStyle::Outline)) openForm(FormKind::AddCarriage);
    right -= 94.0f;
    if (ui.button(306, right, rowY, 82.0f, 44.0f, "Sửa tàu", ButtonStyle::Outline)) openEditTrain(current->code);

    const float codeW = ui.text(detailX, 152.0f, current->code, FontCondBold, 30.0f, Palette::Ink);
    ui.text(detailX + codeW + 12.0f, 154.0f,
            (source ? source->getTrainTypeName() + " · " : "") +
            string(current->direction == 1 ? "Bắc → Nam" : "Nam → Bắc") +
            " · " + priorityName(current->priority),
            FontRegular, 15.0f, Palette::Muted);

    // Các toa xe của đoàn tàu.
    float chipX = detailX;
    if (source) {
        for (LinkedList<CarriageNode*>::Node* c = source->getHeadCarriage(); c; c = c->next) {
            const string label = "Toa " + to_string(c->data->getCarriageNo()) + " · " + c->data->getType();
            const float w = ui.textWidth(label, FontRegular, 14.0f) + 18.0f;
            if (chipX + w > detailX + detailW) break;
            if (ui.button(3100 + c->data->getCarriageNo(), chipX, 172.0f, w, 24.0f,
                          label, ButtonStyle::Outline)) {
                openEditCarriage(current->code, c->data->getCarriageNo());
            }
            chipX += w + 8.0f;
        }
    }
    const float searchX = detailX + detailW - 100.0f;
    ui.text(searchX - 8.0f, 184.0f, "Tìm ID ga", FontRegular, 14.0f, Palette::Ink, Align::Right);
    ui.field(305, searchX, 170.0f, 100.0f, 28.0f, stopSearch, "VD: 105", FontRegular, 14.0f);

    const float tableY = 204.0f;
    const float colId = detailX + 12.0f;
    const float colName = colId + 60.0f;
    const float colArr = colName + 190.0f;
    const float colDep = colArr + 130.0f;
    const float colBase = colDep + 130.0f;
    const float colRay = colBase + 140.0f;
    const float colLate = colRay + 90.0f;
    const float stopH = 26.0f;

    ui.rect(detailX, tableY, detailW, 34.0f + 18.0f * stopH, Palette::White);
    ui.rect(detailX, tableY, detailW, 2.0f, Palette::Ink);
    const float headY = tableY + 17.0f;
    ui.text(colId, headY, "ID", FontCondSemi, 16.0f, Palette::Ink);
    ui.text(colName, headY, "Ga dừng", FontCondSemi, 16.0f, Palette::Ink);
    ui.text(colArr, headY, "Giờ đến", FontCondSemi, 16.0f, Palette::Ink);
    ui.text(colDep, headY, "Giờ đi", FontCondSemi, 16.0f, Palette::Ink);
    ui.text(colBase, headY, "Lịch công bố", FontCondSemi, 16.0f, Palette::Ink);
    ui.text(colRay, headY, "Ray", FontCondSemi, 16.0f, Palette::Ink);
    ui.text(colLate, headY, "Chờ thêm", FontCondSemi, 16.0f, Palette::Ink);
    ui.rect(detailX, tableY + 32.0f, detailW, 1.0f, Palette::Ink);

    int row = 0;
    for (int s = 0; s < current->stopCount && row < 18; ++s) {
        const SimStop& stop = current->stops[s];
        if (!stopSearch.empty() && to_string(stop.stationID).find(trim(stopSearch)) == string::npos) continue;
        const float top = tableY + 33.0f + row * stopH;
        const float mid = top + stopH / 2.0f;
        const bool over = ui.hovered(detailX, top, detailW, stopH);
        const float amount = ui.anim(3200 + s, over ? 1.0f : 0.0f);
        sf::Color tint = Palette::Hover;
        tint.a = static_cast<uint8_t>(255.0f * amount);
        ui.rect(detailX, top, detailW, stopH - 1.0f, tint);
        if (over) ui.markHand();
        if (ui.clicked(detailX, top, detailW, stopH)) openEditStop(current->code, stop.stationID);

        ui.text(colId, mid, to_string(stop.stationID), FontRegular, 14.0f, Palette::Muted);
        ui.text(colName, mid, model.nameOf(stop.stationID), FontSemi, 15.0f, Palette::Ink);
        ui.text(colArr, mid, formatMinute(stop.arrival), FontRegular, 14.0f, Palette::Ink);
        ui.text(colDep, mid, formatMinute(stop.departure), FontRegular, 14.0f, Palette::Ink);
        ui.text(colBase, mid, stop.hasBase ? formatMinute(stop.baseDeparture) : "—", FontRegular, 14.0f, Palette::Muted);
        ui.text(colRay, mid, stop.track > 1 ? "Ray " + to_string(stop.track) : "Ray chính", FontRegular, 14.0f, Palette::Ink);
        const int wait = model.addedWait(*current, s);
        if (wait > 0) ui.text(colLate, mid, formatDuration(wait), FontSemi, 14.0f, Palette::Red);
        if (amount > 0.5f) ui.text(detailX + detailW - 12.0f, mid, "Sửa", FontSemi, 14.0f, Palette::Blue, Align::Right);
        ui.rect(detailX, top + stopH - 1.0f, detailW, 1.0f, Palette::RuleSoft);
        ++row;
    }
    ui.rect(detailX, tableY + 33.0f + 18.0f * stopH, detailW, 2.0f, Palette::Ink);
}

// ==================== MÀN BẢO TRÌ ====================
void RailwayApp::drawMaintenance() {
    const float rowY = 80.0f;
    ui.text(32.0f, rowY + 22.0f, to_string(model.windowCount()) + " lịch bảo trì", FontCondSemi, 22.0f, Palette::Ink);

    if (ui.button(401, 1110.0f, rowY, 218.0f, 44.0f, "Đóng ga, khu gian hoặc ray", ButtonStyle::Solid)) openForm(FormKind::CloseResource);
    if (ui.button(402, 990.0f, rowY, 108.0f, 44.0f, "Báo tàu trễ", ButtonStyle::Outline)) openForm(FormKind::ReportDelay);

    const float tableX = 32.0f;
    const float tableW = 1296.0f;
    const float tableY = 134.0f;
    const float rowH = 44.0f;
    const int visible = 11;
    const float colMark = tableX + 12.0f;
    const float colKind = colMark + 44.0f;
    const float colWhere = colKind + 130.0f;
    const float colFrom = colWhere + 420.0f;
    const float colTo = colFrom + 150.0f;
    const float colState = colTo + 150.0f;

    if (ui.hovered(tableX, tableY, tableW, 33.0f + visible * rowH)) windowScroll -= static_cast<int>(ui.wheel());
    windowScroll = max(0, min(windowScroll, max(0, model.windowCount() - visible)));

    ui.rect(tableX, tableY, tableW, 34.0f + visible * rowH, Palette::White);
    ui.rect(tableX, tableY, tableW, 2.0f, Palette::Ink);
    const float headY = tableY + 17.0f;
    ui.text(colKind, headY, "Loại", FontCondSemi, 16.0f, Palette::Ink);
    ui.text(colWhere, headY, "Vị trí", FontCondSemi, 16.0f, Palette::Ink);
    ui.text(colFrom, headY, "Từ", FontCondSemi, 16.0f, Palette::Ink);
    ui.text(colTo, headY, "Đến", FontCondSemi, 16.0f, Palette::Ink);
    ui.text(colState, headY, "Trạng thái", FontCondSemi, 16.0f, Palette::Ink);
    ui.rect(tableX, tableY + 32.0f, tableW, 1.0f, Palette::Ink);

    if (model.windowCount() == 0) {
        ui.text(tableX + tableW / 2.0f, tableY + 120.0f, "Hiện chưa có lịch bảo trì nào.", FontRegular, 15.0f, Palette::Muted, Align::Center);
    }
    const double now = simulation.minute();
    const char* const kinds[3] = {"Ga", "Khu gian", "Ray"};
    for (int r = 0; r < visible && r + windowScroll < model.windowCount(); ++r) {
        const int index = r + windowScroll;
        const SimWindow& w = model.window(index);
        const float top = tableY + 33.0f + r * rowH;
        const float mid = top + rowH / 2.0f;
        ui.hazard(colMark, mid - 5.0f, 30.0f, 10.0f);
        ui.text(colKind, mid, kinds[max(0, min(2, w.type))], FontRegular, 15.0f, Palette::Ink);
        ui.textFit(colWhere, mid, w.label, FontSemi, 15.0f, Palette::Ink, 410.0f);
        ui.text(colFrom, mid, formatMinute(w.start), FontRegular, 15.0f, Palette::Ink);
        ui.text(colTo, mid, formatMinute(w.end), FontRegular, 15.0f, Palette::Ink);
        const bool activeNow = now >= w.start && now < w.end;
        const string state = activeNow ? "Đang bảo trì" : (now < w.start ? "Sắp tới" : "Đã xong");
        ui.text(colState, mid, state, activeNow ? FontSemi : FontRegular, 15.0f, activeNow ? Palette::Red : Palette::Muted);
        if (ui.button(4100 + index, tableX + tableW - 112.0f, top + 5.0f, 100.0f, 34.0f, "Mở lại", ButtonStyle::Outline)) {
            confirmAction = ConfirmAction::ReopenWindow;
            confirmWindow = index;
            confirmText = "Mở lại " + w.label + " (" + formatMinute(w.start) + " – " + formatMinute(w.end) + ")?";
            openForm(FormKind::Confirm);
        }
        ui.rect(tableX, top + rowH - 1.0f, tableW, 1.0f, Palette::RuleSoft);
    }
    ui.rect(tableX, tableY + 33.0f + visible * rowH, tableW, 2.0f, Palette::Ink);
    drawLog(tableX, tableY + 33.0f + visible * rowH + 26.0f, tableW);
}

// ==================== MÀN TIẾN TRÌNH TÀU ====================
void RailwayApp::drawChart() {
    int order[SIM_MAX_TRAINS];
    int delays[SIM_MAX_TRAINS];
    int lateCount = 0;
    for (int t = 0; t < model.trainCount(); ++t) {
        const SimTrain& train = model.train(t);
        const int total = train.stopCount ? model.stopDelay(train, train.stopCount - 1) : 0;
        delays[t] = total;
        if (total > 0) ++lateCount;
        int slot = t;
        while (slot > 0 && delays[order[slot - 1]] < total) {
            order[slot] = order[slot - 1];
            --slot;
        }
        order[slot] = t;
    }

    string headline = "Chưa có đoàn tàu nào";
    if (model.trainCount() > 0) {
        const SimTrain& worst = model.train(order[0]);
        if (lateCount == model.trainCount()) {
            headline = "Cả " + to_string(lateCount) + " tàu đều trễ, " + worst.code + " trễ nhiều nhất: " + formatDuration(delays[order[0]]);
        } else if (lateCount > 0) {
            headline = to_string(lateCount) + " trên " + to_string(model.trainCount()) + " tàu bị trễ, " + worst.code + " trễ nhiều nhất: " + formatDuration(delays[order[0]]);
        } else {
            headline = "Tất cả các tàu đều đúng giờ";
        }
    }
    ui.text(32.0f, 96.0f, headline, FontCondBold, 30.0f, Palette::Ink);
    ui.text(32.0f, 124.0f, "Mỗi hàng là một tàu, thời gian chạy từ trái sang phải. Đoạn đỏ là lúc tàu phải chờ thêm so với lịch.",
            FontRegular, 15.0f, Palette::Body);
    if (ui.button(500, 1188.0f, 80.0f, 70.0f, 44.0f, "Bản đồ", ButtonStyle::Segment, false)) screen = AppScreen::Map;
    ui.button(501, 1258.0f, 80.0f, 70.0f, 44.0f, "Đồ thị", ButtonStyle::Segment, true);

    float legendX = 32.0f;
    const float legendY = 148.0f;
    ui.rect(legendX, legendY - 6.0f, 24.0f, 12.0f, Palette::Blue);
    legendX += 30.0f;
    legendX += ui.text(legendX, legendY, "Tàu chạy, gồm các lần dừng ga theo lịch", FontRegular, 14.0f, Palette::Body) + 20.0f;
    ui.rect(legendX, legendY - 6.0f, 24.0f, 12.0f, Palette::WaitRed);
    legendX += 30.0f;
    legendX += ui.text(legendX, legendY, "Chờ thêm do điều phối", FontRegular, 14.0f, Palette::Body) + 20.0f;
    ui.rect(legendX, legendY - 2.0f, 24.0f, 4.0f, Palette::Stone);
    legendX += 30.0f;
    ui.text(legendX, legendY, "Lịch công bố", FontRegular, 14.0f, Palette::Body);

    const float chartY = 160.0f;
    const float labelX = 44.0f;
    const float plotX = 180.0f;
    const float plotW = 1000.0f;
    const float lateX = 1316.0f;
    const int rows = min(model.trainCount(), 8);
    const float rowH = rows > 0 ? max(46.0f, min(72.0f, 288.0f / rows)) : 72.0f;
    const float axisH = 44.0f;
    const float chartH = 2.0f + axisH + rows * rowH;

    int rangeStart = model.firstMinute();
    int rangeEnd = model.lastMinute();
    rangeStart = rangeStart / 360 * 360;
    rangeEnd = (rangeEnd + 359) / 360 * 360;
    if (rangeEnd <= rangeStart) rangeEnd = rangeStart + 360;
    const auto X = [&](double minute) { return plotX + static_cast<float>((minute - rangeStart) / (rangeEnd - rangeStart)) * plotW; };

    ui.rect(32.0f, chartY, 1296.0f, chartH, Palette::White);
    ui.rect(32.0f, chartY, 1296.0f, 2.0f, Palette::Ink);
    ui.rect(32.0f, chartY + chartH, 1296.0f, 2.0f, Palette::Ink);

    const int tickStep = rangeEnd - rangeStart > 5000 ? 720 : 360;
    for (int day = rangeStart / 1440; day * 1440 < rangeEnd; ++day) {
        const int from = max(rangeStart, day * 1440);
        const int to = min(rangeEnd, (day + 1) * 1440);
        ui.text(X((from + to) / 2.0), chartY + 16.0f, "D+" + to_string(day), FontSemi, 15.0f, Palette::Ink, Align::Center);
    }
    const float rowsTop = chartY + 2.0f + axisH;
    for (int tick = rangeStart; tick <= rangeEnd; tick += tickStep) {
        const float tx = X(tick);
        ui.text(tx, chartY + 36.0f, formatMinute(tick % 1440), FontRegular, 13.0f, Palette::Muted, Align::Center);
        ui.rect(tx, rowsTop, 1.0f, rows * rowH, tick % 1440 == 0 ? Palette::Rule : sf::Color(238, 240, 241));
    }

    struct WaitRow { int train; int stop; int minutes; };
    WaitRow waits[64];
    int waitCount = 0;

    for (int r = 0; r < rows; ++r) {
        const int t = order[r];
        const SimTrain& train = model.train(t);
        const float y = rowsTop + r * rowH;
        ui.rect(32.0f, y, 1296.0f, 1.0f, Palette::RuleSoft);
        ui.text(labelX, y + rowH / 2.0f - 9.0f, train.code, FontCondBold, 24.0f, Palette::Ink);
        if (train.stopCount < 2) continue;
        const SimStop& first = train.stops[0];
        const SimStop& last = train.stops[train.stopCount - 1];
        ui.textFit(labelX, y + rowH / 2.0f + 13.0f, model.nameOf(first.stationID) + " → " + model.nameOf(last.stationID),
                   FontRegular, 13.0f, Palette::Muted, 128.0f);
        ui.text(lateX, y + rowH / 2.0f - 9.0f, delays[t] > 0 ? "Trễ " + formatDuration(delays[t]) : "Đúng giờ", FontSemi, 15.0f, Palette::Ink, Align::Right);
        ui.text(lateX, y + rowH / 2.0f + 11.0f, "Đến " + formatMinute(last.arrival), FontRegular, 13.0f, Palette::Muted, Align::Right);

        ui.rect(X(first.baseDeparture), y + 10.0f, X(last.baseDeparture) - X(first.baseDeparture), 4.0f, Palette::Stone);

        const auto segment = [&](double from, double to, sf::Color color, const string& tip) {
            if (to <= from) return;
            const float left = X(from);
            const float width = max(4.0f, X(to) - left - 2.0f);
            ui.rect(left, y + 20.0f, width, 24.0f, color);
            if (ui.hovered(left, y + 20.0f, width, 24.0f)) ui.tooltip(left + width / 2.0f, y + 18.0f, tip);
        };

        double cursor = first.departure;
        for (int s = 1; s < train.stopCount - 1; ++s) {
            const int wait = model.addedWait(train, s);
            if (wait <= 0) continue;
            const double to = train.stops[s].departure;
            const double from = to - wait;
            const string station = model.nameOf(train.stops[s].stationID);
            segment(cursor, from, Palette::Blue, "Chạy từ " + formatMinute(cursor) + " đến " + formatMinute(from));
            segment(from, to, Palette::WaitRed, "Chờ ở " + station + " " + formatDuration(wait) + ", từ " + formatMinute(from) + " đến " + formatMinute(to));
            if (wait >= 60 && rowH >= 66.0f) {
                const string note = station + " · " + formatDuration(wait);
                const float noteW = ui.textWidth(note, FontSemi, 13.0f) + 6.0f;
                const float noteX = X((from + to) / 2.0) - noteW / 2.0f;
                ui.rect(noteX, y + 47.0f, noteW, 18.0f, Palette::White);
                ui.text(noteX + 3.0f, y + 56.0f, note, FontSemi, 13.0f, Palette::Ink);
            }
            if (waitCount < 64) waits[waitCount++] = {t, s, wait};
            cursor = to;
        }
        segment(cursor, last.arrival, Palette::Blue, "Chạy từ " + formatMinute(cursor) + " đến " + formatMinute(last.arrival));
    }

    for (int i = 1; i < waitCount; ++i) {
        const WaitRow item = waits[i];
        int j = i - 1;
        while (j >= 0 && waits[j].minutes < item.minutes) {
            waits[j + 1] = waits[j];
            --j;
        }
        waits[j + 1] = item;
    }

    float y = chartY + chartH + 24.0f;
    ui.text(32.0f, y, "Vì sao tàu phải chờ", FontCondSemi, 22.0f, Palette::Ink);
    y += 16.0f;
    const int shown = min(waitCount, 10);
    const float columnW = 632.0f;
    if (shown == 0) ui.text(32.0f, y + 15.0f, "Không có tàu nào phải chờ thêm.", FontRegular, 15.0f, Palette::Muted);
    for (int i = 0; i < shown; ++i) {
        const int column = i % 2;
        const float cx = 32.0f + column * (columnW + 32.0f);
        const float cy = y + (i / 2) * 30.0f;
        const float mid = cy + 15.0f;
        ui.rect(cx, cy, columnW, 1.0f, Palette::Rule);
        const SimTrain& train = model.train(waits[i].train);
        ui.text(cx, mid, train.code, FontSemi, 15.0f, Palette::Ink);
        ui.textFit(cx + 52.0f, mid, model.nameOf(train.stops[waits[i].stop].stationID), FontRegular, 15.0f, Palette::Ink, 90.0f);
        ui.text(cx + 150.0f, mid, formatDuration(waits[i].minutes), FontSemi, 15.0f, Palette::Ink);
        const float ratio = static_cast<float>(waits[i].minutes) / max(1, waits[0].minutes);
        ui.rect(cx + 262.0f, mid - 5.0f, max(3.0f, 80.0f * ratio), 10.0f, Palette::WaitRed);
        const SimStop& stop = train.stops[waits[i].stop];
        ui.textFit(cx + 352.0f, mid, model.waitReason(waits[i].train, waits[i].stop, stop.arrival, true),
                   FontRegular, 15.0f, Palette::Body, columnW - 352.0f);
    }

    ui.text(32.0f, 742.0f,
            "Giờ công bố: Đường sắt Việt Nam, tra ngày 08/11/2026. Thời gian chờ do bộ điều phối tính trên mô hình " +
            to_string(model.stationCount()) + " ga và " + to_string(model.windowCount()) + " lịch bảo trì đang khai báo.",
            FontRegular, 13.0f, Palette::Muted);
}

// ==================== BIỂU MẪU NHẬP LIỆU ====================
void RailwayApp::openForm(FormKind kind) {
    formKind = kind;
    for (int i = 0; i < 5; ++i) formValues[i].clear();
    formChoiceA = 0;
    formChoiceB = 0;
    formError.clear();
    ui.clearFocus();
    if (kind == FormKind::AddCarriage || kind == FormKind::AddStop || kind == FormKind::ReportDelay) {
        formValues[0] = selectedTrain;
    }
    if (kind != FormKind::Confirm) ui.focus(FIELD_BASE + (formValues[0].empty() ? 0 : 1));
}

void RailwayApp::openEditStation(int stationID) {
    StationNode* station = stations.findStationByID(stationID);
    if (!station) return;
    openForm(FormKind::EditStation);
    formStation = stationID;
    formValues[0] = station->getName();
    formValues[1] = to_string(station->getKmMarker());
    formValues[2] = to_string(station->getMaxTracks());
    ui.focus(FIELD_BASE);
}

void RailwayApp::openEditTrain(const string& code) {
    TrainNode* train = trains.findTrainByCode(code);
    if (!train) return;
    openForm(FormKind::EditTrain);
    formTrain = code;
    formChoiceA = train->getDirection() == 1 ? 0 : 1;
    formChoiceB = train->getPriority();
}

void RailwayApp::openEditCarriage(const string& code, int carriageNo) {
    TrainNode* train = trains.findTrainByCode(code);
    CarriageNode* carriage = train ? train->findCarriage(carriageNo) : nullptr;
    if (!carriage) return;
    openForm(FormKind::EditCarriage);
    formTrain = code;
    formCarriageNo = carriageNo;
    formValues[0] = carriage->getType();
    ui.focus(FIELD_BASE);
}

void RailwayApp::openEditStop(const string& code, int stationID) {
    const SimTrain* train = nullptr;
    for (int i = 0; i < model.trainCount(); ++i) {
        if (model.train(i).code == code) train = &model.train(i);
    }
    if (!train) return;
    for (int s = 0; s < train->stopCount; ++s) {
        if (train->stops[s].stationID != stationID) continue;
        openForm(FormKind::EditStop);
        formTrain = code;
        formStation = stationID;
        formValues[0] = formatMinute(train->stops[s].arrival);
        formValues[1] = formatMinute(train->stops[s].departure);
        formTrack = train->stops[s].track;
        formDayA = train->stops[s].arrival / 1440;
        formDayB = train->stops[s].departure / 1440;
        ui.focus(FIELD_BASE);
        return;
    }
}

void RailwayApp::closeForm() {
    formKind = FormKind::None;
    confirmAction = ConfirmAction::None;
    ui.clearFocus();
}

void RailwayApp::runConfirm() {
    if (confirmAction == ConfirmAction::RemoveStation) {
        StationNode* station = stations.findStationByID(formStation);
        if (!station) {
            setNotice("Không tìm thấy ga cần xóa.", true);
        } else if (trains.hasStationStop(formStation) ||
                   trains.hasMaintenanceForStation(formStation)) {
            setNotice("Không thể xóa ga đang có tàu dừng hoặc lịch bảo trì.", true);
        } else if (stations.removeStation(formStation)) {
            trains.resolveConflictAndScheduleSiding(&stations);
            saveAll();
            refresh();
            setNotice("Đã xóa ga " + to_string(formStation) + ".");
        } else {
            setNotice("Không thể xóa ga.", true);
        }
    } else if (confirmAction == ConfirmAction::RemoveTrain) {
        const string code = selectedTrain;
        if (trains.removeTrain(code)) {
            trains.resolveConflictAndScheduleSiding(&stations);
            saveAll();
            refresh();
            selectedTrain = model.trainCount() ? model.train(0).code : "";
            setNotice("Đã bãi bỏ chuyến " + code + " và cập nhật điều phối.");
        } else {
            setNotice("Không tìm thấy tàu " + code + ".", true);
        }
    } else if (confirmAction == ConfirmAction::RemoveStop) {
        if (trains.removeStopSchedule(formTrain, formStation)) {
            trains.resolveConflictAndScheduleSiding(&stations);
            saveAll();
            refresh();
            setNotice("Đã bãi bỏ điểm dừng và cập nhật điều phối.");
        } else {
            setNotice("Không thể bãi bỏ: lịch phải còn ít nhất 2 điểm dừng.", true);
        }
    } else if (confirmAction == ConfirmAction::RemoveCarriage) {
        if (trains.removeCarriage(formTrain, formCarriageNo)) {
            saveAll();
            refresh();
            setNotice("Đã xóa toa " + to_string(formCarriageNo) + ".");
        } else {
            setNotice("Không tìm thấy toa cần xóa.", true);
        }
    } else if (confirmAction == ConfirmAction::ReopenWindow && confirmWindow >= 0 &&
               confirmWindow < model.windowCount()) {
        const SimWindow w = model.window(confirmWindow);
        const char* const resources[3] = {"STATION", "BLOCK", "TRACK"};
        string error;
        if (changeMaintenance(true, resources[w.type], w.first, w.second, w.start, w.end, error)) {
            setNotice("Đã mở lại " + w.label + " và cập nhật điều phối.");
        } else {
            setNotice(error, true);
        }
    }
    closeForm();
}

void RailwayApp::submitForm() {
    formError.clear();
    switch (formKind) {
    case FormKind::AddStation: {
        int id = 0;
        int tracks = 0;
        double km = 0.0;
        if (!parseInteger(formValues[0], id) || !parseReal(formValues[2], km) || !parseInteger(formValues[3], tracks)) {
            formError = "ID, lý trình và số ray phải là số.";
            return;
        }
        if (!stations.addStation(id, trim(formValues[1]), km, tracks)) {
            formError = "ID đã tồn tại, hoặc tên, lý trình, số ray không hợp lệ.";
            return;
        }
        saveAll();
        refresh();
        setNotice("Đã thêm ga " + trim(formValues[1]) + ".");
        break;
    }
    case FormKind::EditStation: {
        int tracks = 0;
        double km = 0.0;
        StationNode* station = stations.findStationByID(formStation);
        if (!station || !parseReal(formValues[1], km) || !parseInteger(formValues[2], tracks)) {
            formError = "Lý trình và số ray phải là số.";
            return;
        }
        const bool kmUnchanged = km == station->getKmMarker();
        const bool routeSafe = !trains.hasStationStop(formStation) || kmUnchanged;
        const bool maintenanceSafe = kmUnchanged ||
            !trains.hasMaintenanceBlockForStation(formStation);
        if (!routeSafe || !maintenanceSafe || trains.hasTrackAbove(formStation, tracks) ||
            trains.hasMaintenanceTrackAbove(formStation, tracks) ||
            !stations.updateStation(formStation, trim(formValues[0]), km, tracks)) {
            formError = "Tên, lý trình hoặc số ray vi phạm ràng buộc tuyến và bảo trì.";
            return;
        }
        trains.resolveConflictAndScheduleSiding(&stations);
        saveAll();
        refresh();
        setNotice("Đã cập nhật ga " + to_string(formStation) + ".");
        break;
    }
    case FormKind::AddTrain: {
        const string code = trim(formValues[0]);
        const int priority = formChoiceB == 0 ? -1 : formChoiceB;
        if (!trains.addTrain(code, formChoiceA == 0 ? 1 : 0, priority)) {
            formError = "Mã tàu trống hoặc đã tồn tại.";
            return;
        }
        saveAll();
        refresh();
        selectedTrain = code;
        setNotice("Đã thêm tàu " + code + ".");
        break;
    }
    case FormKind::EditTrain: {
        const int direction = formChoiceA == 0 ? 1 : 0;
        const int priority = formChoiceB == 0 ? -1 : formChoiceB;
        if (!trains.updateTrain(formTrain, direction, priority, &stations)) {
            formError = "Hướng chạy không phù hợp thứ tự ga hoặc cấp ưu tiên không hợp lệ.";
            return;
        }
        trains.resolveConflictAndScheduleSiding(&stations);
        saveAll();
        refresh();
        selectedTrain = formTrain;
        setNotice("Đã cập nhật tàu " + formTrain + ".");
        break;
    }
    case FormKind::AddCarriage: {
        int number = 0;
        const string code = trim(formValues[0]);
        if (!trains.findTrainByCode(code)) { formError = "Không tìm thấy tàu " + code + "."; return; }
        if (!parseInteger(formValues[1], number)) { formError = "Số toa phải là số nguyên."; return; }
        if (!trains.addCarriageToTrain(code, number, trim(formValues[2]))) {
            formError = "Số toa đã tồn tại hoặc loại toa còn trống.";
            return;
        }
        saveAll();
        refresh();
        setNotice("Đã gán toa " + to_string(number) + " vào tàu " + code + ".");
        break;
    }
    case FormKind::EditCarriage: {
        if (!trains.updateCarriage(formTrain, formCarriageNo, trim(formValues[0]))) {
            formError = "Loại toa không được để trống.";
            return;
        }
        saveAll();
        refresh();
        setNotice("Đã cập nhật toa " + to_string(formCarriageNo) + ".");
        break;
    }
    case FormKind::AddStop: {
        int station = 0;
        int arrival = 0;
        int departure = 0;
        const string code = trim(formValues[0]);
        if (!parseInteger(formValues[1], station)) { formError = "ID ga phải là số nguyên."; return; }
        if (!parseTime(formValues[2], 0, arrival) || !parseTime(formValues[3], 0, departure)) {
            formError = "Giờ phải có dạng 09:20 hoặc D+1 01:00.";
            return;
        }
        if (!trains.addStopScheduleToTrain(code, station, arrival, departure, 1, &stations)) {
            formError = "Tàu hoặc ga không tồn tại, giờ sai thứ tự, ga bị trùng hoặc sai hướng tuyến.";
            return;
        }
        saveAll();
        refresh();
        setNotice("Đã thêm điểm dừng và kiểm tra xung đột.");
        break;
    }
    case FormKind::EditStop: {
        int arrival = 0;
        int departure = 0;
        if (!parseTime(formValues[0], formDayA, arrival) || !parseTime(formValues[1], formDayB, departure)) {
            formError = "Giờ phải có dạng 09:20 hoặc D+1 01:00.";
            return;
        }
        if (!trains.updateStopSchedule(formTrain, formStation, arrival, departure, formTrack, &stations)) {
            formError = "Giờ nằm ngoài khoảng cho phép hoặc số ray không hợp lệ.";
            return;
        }
        trains.resolveConflictAndScheduleSiding(&stations);
        saveAll();
        refresh();
        setNotice("Đã điều chỉnh lịch dừng và chạy lại điều phối.");
        break;
    }
    case FormKind::CloseResource: {
        const char* const resources[3] = {"STATION", "BLOCK", "TRACK"};
        int first = 0;
        int second = 0;
        int start = 0;
        int end = 0;
        if (!parseInteger(formValues[0], first) || (formChoiceA != 0 && !parseInteger(formValues[1], second))) {
            formError = "ID ga và số ray phải là số nguyên.";
            return;
        }
        if (!parseTime(formValues[2], 0, start) || !parseTime(formValues[3], 0, end)) {
            formError = "Giờ phải có dạng 09:20 hoặc D+1 01:00.";
            return;
        }
        if (!changeMaintenance(false, resources[formChoiceA], first, second, start, end, formError)) return;
        setNotice("Đã đóng tài nguyên và cập nhật điều phối.");
        break;
    }
    case FormKind::ReportDelay: {
        int station = 0;
        int minutes = 0;
        const string code = trim(formValues[0]);
        if (!parseInteger(formValues[1], station) || !parseInteger(formValues[2], minutes)) {
            formError = "ID ga và số phút trễ phải là số nguyên.";
            return;
        }
        if (!trains.applyDelayAndReschedule(code, station, minutes, &stations)) {
            formError = "Không tìm thấy tàu " + code + " hoặc ga không nằm trong lịch.";
            return;
        }
        saveAll();
        refresh();
        setNotice("Đã cộng " + to_string(minutes) + " phút trễ cho tàu " + code + " và tái lập lịch.");
        break;
    }
    case FormKind::Confirm:
        runConfirm();
        return;
    case FormKind::None:
        return;
    }
    closeForm();
}

void RailwayApp::drawForm() {
    struct FieldDef {
        const char* label;
        const char* hint;
    };

    string title;
    string subtitle;
    string submit = "Lưu";
    FieldDef defs[4];
    int fields = 0;
    float height = 360.0f;
    bool hasChoiceA = false;
    bool hasChoiceB = false;

    switch (formKind) {
    case FormKind::AddStation:
        title = "Thêm ga mới";
        subtitle = "Ga được đặt lên bản đồ theo lý trình";
        submit = "Thêm ga";
        defs[0] = {"ID ga", "VD: 119"};
        defs[1] = {"Tên ga", "VD: Tháp Chàm"};
        defs[2] = {"Lý trình (km)", "VD: 1408"};
        defs[3] = {"Số ray", "1 = ga xép"};
        fields = 4;
        break;
    case FormKind::EditStation:
        title = "Sửa thông tin ga";
        subtitle = "Ga " + to_string(formStation);
        submit = "Lưu thay đổi";
        defs[0] = {"Tên ga", "VD: Tháp Chàm"};
        defs[1] = {"Lý trình (km)", "VD: 1408"};
        defs[2] = {"Số ray", "1 = ga xép"};
        fields = 3;
        height = 340.0f;
        break;
    case FormKind::AddTrain:
        title = "Thêm đoàn tàu";
        subtitle = "Sau đó thêm toa và điểm dừng cho tàu";
        submit = "Thêm tàu";
        defs[0] = {"Mã tàu", "VD: SE5"};
        fields = 1;
        hasChoiceA = hasChoiceB = true;
        height = 390.0f;
        break;
    case FormKind::EditTrain:
        title = "Sửa đoàn tàu " + formTrain;
        subtitle = "Đổi hướng chạy hoặc cấp ưu tiên";
        submit = "Lưu thay đổi";
        hasChoiceA = hasChoiceB = true;
        height = 330.0f;
        break;
    case FormKind::AddCarriage:
        title = "Gán toa xe";
        submit = "Gán toa";
        defs[0] = {"Tàu", "VD: SE1"};
        defs[1] = {"Số toa", "VD: 3"};
        defs[2] = {"Loại toa", "VD: GiuongNam"};
        fields = 3;
        break;
    case FormKind::EditCarriage:
        title = "Sửa toa xe";
        subtitle = "Tàu " + formTrain + " · Toa " + to_string(formCarriageNo);
        submit = "Lưu loại toa";
        defs[0] = {"Loại toa", "VD: NgoiMem hoặc GiuongNam"};
        fields = 1;
        height = 300.0f;
        break;
    case FormKind::AddStop:
        title = "Thêm điểm dừng";
        subtitle = "Điểm dừng mới được thêm vào cuối hành trình";
        submit = "Thêm và kiểm tra xung đột";
        defs[0] = {"Tàu", "VD: SE1"};
        defs[1] = {"ID ga", "VD: 105"};
        defs[2] = {"Giờ đến", "D+1 01:00"};
        defs[3] = {"Giờ đi", "D+1 01:10"};
        fields = 4;
        break;
    case FormKind::CloseResource:
        title = "Đóng tài nguyên bảo trì";
        subtitle = "Tàu sẽ được điều phối tránh khoảng thời gian này";
        submit = "Đóng và điều phối lại";
        defs[0] = {formChoiceA == 1 ? "Ga đầu khu gian (ID)" : "ID ga", "VD: 105"};
        defs[1] = {formChoiceA == 0 ? "Không cần nhập" : (formChoiceA == 1 ? "Ga kế tiếp (ID)" : "Số ray"), formChoiceA == 1 ? "VD: 106" : "VD: 2"};
        defs[2] = {"Bắt đầu", "D+1 02:00"};
        defs[3] = {"Kết thúc", "D+1 04:00"};
        fields = 4;
        hasChoiceA = true;
        height = 420.0f;
        break;
    case FormKind::ReportDelay:
        title = "Báo tàu trễ";
        subtitle = "Các ga phía sau được dời giờ và tái lập lịch";
        submit = "Báo trễ";
        defs[0] = {"Tàu", "VD: SE1"};
        defs[1] = {"ID ga bắt đầu trễ", "VD: 102"};
        defs[2] = {"Số phút trễ", "VD: 30"};
        fields = 3;
        break;
    case FormKind::EditStop:
        title = "Điều chỉnh lịch dừng";
        subtitle = "Tàu " + formTrain + " tại ga " + model.nameOf(formStation);
        submit = "Lưu và chạy lại điều phối";
        {
            const SimStation* station = model.findStation(formStation);
            const int rows = station ? (min(station->tracks, 12) + 1) / 2 : 1;
            height = 442.0f + rows * 46.0f;
        }
        break;
    case FormKind::Confirm:
        title = "Xác nhận";
        submit = "Đồng ý";
        height = 230.0f;
        break;
    case FormKind::None:
        return;
    }

    ui.rect(0.0f, 0.0f, DESIGN_W, DESIGN_H, sf::Color(20, 23, 26, 140));
    const float panelW = 560.0f;
    const float panelX = (DESIGN_W - panelW) / 2.0f;
    const float panelY = (DESIGN_H - height) / 2.0f;
    ui.rect(panelX, panelY, panelW, height, Palette::White);
    const float bodyX = panelX + 32.0f;
    const float bodyW = panelW - 64.0f;
    float y = panelY + 44.0f;
    ui.text(bodyX, y, title, FontCondBold, 32.0f, Palette::Ink);
    y += 28.0f;
    if (!subtitle.empty()) {
        ui.text(bodyX, y, subtitle, FontRegular, 15.0f, Palette::Muted);
        y += 18.0f;
    }
    ui.rect(bodyX, y, bodyW, 2.0f, Palette::Ink);
    y += 16.0f;

    const float gap = 16.0f;
    const float colW = (bodyW - gap) / 2.0f;
    int focusable = fields;

    if (formKind == FormKind::Confirm) {
        ui.textFit(bodyX, y + 16.0f, confirmText, FontRegular, 15.0f, Palette::Ink, bodyW);
    } else if (formKind == FormKind::EditStop) {
        focusable = 2;
        string readonlyTrain = formTrain;
        string readonlyStation = to_string(formStation) + " · " + model.nameOf(formStation);
        ui.text(bodyX, y + 8.0f, "Tàu", FontSemi, 15.0f, Palette::Ink);
        ui.text(bodyX + colW + gap, y + 8.0f, "Ga dừng", FontSemi, 15.0f, Palette::Ink);
        ui.field(FIELD_BASE + 8, bodyX, y + 20.0f, colW, 44.0f, readonlyTrain, "", FontRegular, 15.0f, true);
        ui.field(FIELD_BASE + 9, bodyX + colW + gap, y + 20.0f, colW, 44.0f, readonlyStation, "", FontRegular, 15.0f, true);
        y += 76.0f;
        ui.text(bodyX, y + 8.0f, "Giờ đến", FontSemi, 15.0f, Palette::Ink);
        ui.text(bodyX + colW + gap, y + 8.0f, "Giờ đi", FontSemi, 15.0f, Palette::Ink);
        ui.field(FIELD_BASE, bodyX, y + 20.0f, colW, 60.0f, formValues[0], "", FontCondSemi, 34.0f);
        ui.field(FIELD_BASE + 1, bodyX + colW + gap, y + 20.0f, colW, 60.0f, formValues[1], "", FontCondSemi, 34.0f);
        y += 92.0f;

        const SimTrain* train = nullptr;
        for (int i = 0; i < model.trainCount(); ++i) {
            if (model.train(i).code == formTrain) train = &model.train(i);
        }
        string hint;
        if (train) {
            for (int s = 0; s < train->stopCount; ++s) {
                if (train->stops[s].stationID != formStation) continue;
                if (s > 0) {
                    hint = "Từ " + formatMinute(train->stops[s - 1].departure) + " (rời " + model.nameOf(train->stops[s - 1].stationID) + ")";
                }
                if (s + 1 < train->stopCount) {
                    hint += string(hint.empty() ? "Đến " : " đến ") + formatMinute(train->stops[s + 1].arrival) +
                            " (đến " + model.nameOf(train->stops[s + 1].stationID) + ")";
                }
            }
        }
        ui.textFit(bodyX, y, hint + ".", FontRegular, 14.0f, Palette::Muted, bodyW);
        y += 18.0f;
        ui.text(bodyX, y, "Ngày sau ghi dạng D+1 01:00.", FontRegular, 14.0f, Palette::Muted);
        y += 26.0f;

        ui.text(bodyX, y, "Ray sử dụng tại " + model.nameOf(formStation), FontSemi, 15.0f, Palette::Ink);
        y += 14.0f;
        const SimStation* station = model.findStation(formStation);
        const int tracks = station ? min(station->tracks, 12) : 1;
        int typedArrival = 0;
        int typedDeparture = 0;
        const bool timesOk = parseTime(formValues[0], formDayA, typedArrival) && parseTime(formValues[1], formDayB, typedDeparture);
        const float cellH = 40.0f;
        for (int r = 1; r <= tracks; ++r) {
            const int index = r - 1;
            const float cx = bodyX + (index % 2) * (colW + gap);
            const float cy = y + (index / 2) * (cellH + 6.0f);
            string occupant = "Trống";
            bool busy = false;
            if (timesOk) {
                for (int t = 0; t < model.trainCount() && !busy; ++t) {
                    const SimTrain& other = model.train(t);
                    if (other.code == formTrain) continue;
                    for (int s = 0; s < other.stopCount; ++s) {
                        const SimStop& stop = other.stops[s];
                        if (stop.stationID == formStation && stop.track == r &&
                            stop.arrival <= typedDeparture && stop.departure >= typedArrival) {
                            occupant = other.code + " " + formatMinute(stop.arrival % 1440) + " – " + formatMinute(stop.departure % 1440);
                            busy = true;
                            break;
                        }
                    }
                }
            }
            const bool selected = formTrack == r;
            const bool over = ui.hovered(cx, cy, colW, cellH);
            const float amount = ui.anim(9300 + r, over ? 1.0f : 0.0f);
            if (over) ui.markHand();
            if (ui.clicked(cx, cy, colW, cellH)) formTrack = r;
            sf::Color tint = Palette::Hover;
            tint.a = static_cast<uint8_t>(255.0f * amount);
            ui.rect(cx, cy, colW, cellH, selected ? Palette::Ink : tint);
            ui.frame(cx, cy, colW, cellH, 1.0f, selected ? Palette::Ink : Palette::Rule);
            const sf::Color ink = selected ? Palette::White : Palette::Ink;
            ui.text(cx + 10.0f, cy + cellH / 2.0f, "R" + to_string(r) + (r == 1 ? " chính" : " phụ"), FontSemi, 15.0f, ink);
            ui.textFit(cx + 78.0f, cy + cellH / 2.0f, occupant, FontRegular, 14.0f,
                       selected ? Palette::White : (busy ? Palette::Amber : Palette::Muted), colW - 86.0f);
        }
        y += ((tracks + 1) / 2) * (cellH + 6.0f) + 2.0f;
        ui.text(bodyX, y + 6.0f, "Khi lưu, hệ thống chạy lại điều phối toàn tuyến và ghi vào trains.txt.", FontRegular, 14.0f, Palette::Muted);
    } else {
        if (formKind == FormKind::CloseResource) {
            const char* const labels[3] = {"Ga", "Khu gian", "Ray"};
            for (int i = 0; i < 3; ++i) {
                if (ui.button(9400 + i, bodyX + i * 96.0f, y, 96.0f, 44.0f, labels[i], ButtonStyle::Segment, formChoiceA == i)) formChoiceA = i;
            }
            y += 60.0f;
        }
        for (int i = 0; i < fields; ++i) {
            const bool wide = formKind == FormKind::AddTrain || (formKind == FormKind::AddCarriage && i == 2) ||
                              (formKind == FormKind::ReportDelay && i == 2);
            const float cx = wide ? bodyX : bodyX + (i % 2) * (colW + gap);
            const float cw = wide ? bodyW : colW;
            const bool disabled = formKind == FormKind::CloseResource && i == 1 && formChoiceA == 0;
            ui.text(cx, y + 8.0f, defs[i].label, FontSemi, 15.0f, disabled ? Palette::Muted : Palette::Ink);
            ui.field(FIELD_BASE + i, cx, y + 20.0f, cw, 44.0f, formValues[i], disabled ? "" : defs[i].hint, FontRegular, 15.0f, disabled);
            if (wide || i % 2 == 1 || i == fields - 1) y += 78.0f;
        }
        if (hasChoiceA && (formKind == FormKind::AddTrain || formKind == FormKind::EditTrain)) {
            ui.text(bodyX, y + 2.0f, "Hướng chạy", FontSemi, 15.0f, Palette::Ink);
            const char* const labels[2] = {"Bắc → Nam", "Nam → Bắc"};
            for (int i = 0; i < 2; ++i) {
                if (ui.button(9410 + i, bodyX + i * 120.0f, y + 14.0f, 120.0f, 44.0f, labels[i], ButtonStyle::Segment, formChoiceA == i)) formChoiceA = i;
            }
            y += 70.0f;
        }
        if (hasChoiceB) {
            ui.text(bodyX, y + 2.0f, "Cấp ưu tiên", FontSemi, 15.0f, Palette::Ink);
            const char* const labels[4] = {"Theo mã tàu", "P1", "P2", "P3"};
            float bx = bodyX;
            for (int i = 0; i < 4; ++i) {
                const float bw = i == 0 ? 120.0f : 56.0f;
                if (ui.button(9420 + i, bx, y + 14.0f, bw, 44.0f, labels[i], ButtonStyle::Segment, formChoiceB == i)) formChoiceB = i;
                bx += bw;
            }
        }
    }

    if (ui.keyTab() && focusable > 0) {
        int current = ui.focused() - FIELD_BASE;
        current = (current + 1) % focusable;
        if (formKind == FormKind::CloseResource && current == 1 && formChoiceA == 0) current = 2;
        ui.focus(FIELD_BASE + current);
    }

    const float buttonY = panelY + height - 72.0f;
    if (!formError.empty()) {
        ui.textFit(bodyX, buttonY - 16.0f, formError, FontSemi, 14.0f, Palette::Red, bodyW);
    }
    float submitW = bodyW - 124.0f;
    float submitX = bodyX;
    if (formKind == FormKind::EditCarriage) {
        if (ui.button(9502, bodyX, buttonY, 140.0f, 48.0f, "Xóa toa", ButtonStyle::Danger)) {
            confirmAction = ConfirmAction::RemoveCarriage;
            confirmText = "Xóa toa " + to_string(formCarriageNo) + " của tàu " + formTrain + "?";
            formKind = FormKind::Confirm;
            formError.clear();
            ui.clearFocus();
            return;
        }
        submitX += 152.0f;
        submitW -= 152.0f;
    } else if (formKind == FormKind::EditStop) {
        if (ui.button(9502, bodyX, buttonY, 140.0f, 48.0f, "Bãi bỏ điểm dừng", ButtonStyle::Danger)) {
            confirmAction = ConfirmAction::RemoveStop;
            confirmText = "Bãi bỏ điểm dừng " + model.nameOf(formStation) + " của tàu " + formTrain + "?";
            formKind = FormKind::Confirm;
            formError.clear();
            ui.clearFocus();
            return;
        }
        submitX += 152.0f;
        submitW -= 152.0f;
    }
    const bool submitted = ui.button(9500, submitX, buttonY, submitW, 48.0f, submit, ButtonStyle::Solid);
    const bool cancelled = ui.button(9501, bodyX + bodyW - 112.0f, buttonY, 112.0f, 48.0f, "Hủy", ButtonStyle::OutlineInk);
    if (cancelled || ui.keyEscape()) {
        closeForm();
    } else if (submitted || ui.keyEnter()) {
        submitForm();
    }
}

// ==================== VÒNG LẶP ====================
bool RailwayApp::frame(sf::RenderTarget& target, float dt) {
    ui.beginFrame(target, dt);
    target.clear(Palette::Ground);

    const bool modal = formKind != FormKind::None;
    ui.setInputEnabled(!modal);
    simulation.update(ui, model);
    drawHeader();
    switch (screen) {
    case AppScreen::Stations: drawStations(); break;
    case AppScreen::Trains: drawTrains(); break;
    case AppScreen::Maintenance: drawMaintenance(); break;
    case AppScreen::Map:
        if (simulation.drawScreen(ui, map, model)) screen = AppScreen::Chart;
        break;
    case AppScreen::Chart: drawChart(); break;
    }
    ui.setInputEnabled(true);
    if (formKind != FormKind::None) drawForm();
    drawNotice();
    ui.endFrame();
    return running;
}

void GuiMenu::runGUIMenu(StationList& stationList, TrainList& trainList) {
    sf::ContextSettings settings;
    settings.stencilBits = 8;
    settings.antiAliasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode({1360u, 768u}), toSf("Điều hành Đường sắt Tuyến đơn"),
                            sf::Style::Default, sf::State::Windowed, settings);
    window.setFramerateLimit(60);

    RailwayApp* app = new RailwayApp(stationList, trainList);
    app->init();

    const optional<sf::Cursor> arrow = sf::Cursor::createFromSystem(sf::Cursor::Type::Arrow);
    const optional<sf::Cursor> hand = sf::Cursor::createFromSystem(sf::Cursor::Type::Hand);
    bool showingHand = false;

    sf::Clock clock;
    bool keepRunning = true;
    while (window.isOpen() && keepRunning) {
        while (const optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) keepRunning = false;
            if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                window.setView(sf::View(sf::FloatRect({0.0f, 0.0f},
                    {static_cast<float>(resized->size.x), static_cast<float>(resized->size.y)})));
            }
            app->handleEvent(*event);
        }
        app->setMouse(sf::Mouse::getPosition(window));
        if (!app->frame(window, clock.restart().asSeconds())) keepRunning = false;
        window.display();

        if (arrow && hand && app->wantsHand() != showingHand) {
            showingHand = app->wantsHand();
            window.setMouseCursor(showingHand ? *hand : *arrow);
        }
    }
    window.close();
    delete app;
}
