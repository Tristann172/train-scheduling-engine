#include "GraphicalSimulationEngine.h"

#include "StationList.h"
#include "StationNode.h"
#include "StopScheduleNode.h"
#include "TrainList.h"
#include "TrainNode.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>

using namespace std;
using namespace Ui;

namespace {

const float STAGE_W = 560.0f;
const float STAGE_H = 564.0f;
const float PI_F = 3.14159265f;

const float ZOOM_SCALE[4] = {1.0f, 2.9f, 2.9f, 3.6f};
const float ZOOM_X[4] = {0.0f, 150.0f, 200.0f, 225.0f};
const float ZOOM_Y[4] = {0.0f, 32.0f, 165.0f, 342.0f};
const char* const ZOOM_LABEL[4] = {"Toàn quốc", "Miền Bắc", "Miền Trung", "Miền Nam"};

const double SPEED_VALUE[3] = {15.0, 45.0, 120.0};
const char* const SPEED_LABEL[3] = {"Chậm", "Vừa", "Nhanh"};

void appendQuad(sf::VertexArray& va, sf::Vector2f a, sf::Vector2f b, float width, sf::Color color) {
    const sf::Vector2f d = b - a;
    const float length = sqrt(d.x * d.x + d.y * d.y);
    if (length < 0.001f) return;
    const sf::Vector2f n(-d.y / length * width / 2.0f, d.x / length * width / 2.0f);
    va.append(sf::Vertex{a + n, color});
    va.append(sf::Vertex{b + n, color});
    va.append(sf::Vertex{b - n, color});
    va.append(sf::Vertex{a + n, color});
    va.append(sf::Vertex{b - n, color});
    va.append(sf::Vertex{a - n, color});
}

void appendDisc(sf::VertexArray& va, sf::Vector2f center, float radius, sf::Color color) {
    const int segments = 10;
    for (int i = 0; i < segments; ++i) {
        const float a0 = 2.0f * PI_F * i / segments;
        const float a1 = 2.0f * PI_F * (i + 1) / segments;
        va.append(sf::Vertex{center, color});
        va.append(sf::Vertex{center + sf::Vector2f(cos(a0) * radius, sin(a0) * radius), color});
        va.append(sf::Vertex{center + sf::Vector2f(cos(a1) * radius, sin(a1) * radius), color});
    }
}

void strokeLine(sf::VertexArray& va, const sf::Vector2f* pts, int count, float width, sf::Color color) {
    for (int i = 0; i + 1 < count; ++i) appendQuad(va, pts[i], pts[i + 1], width, color);
    for (int i = 1; i + 1 < count; ++i) appendDisc(va, pts[i], width / 2.0f, color);
}

// Vẽ nét đứt dọc đường gấp khúc; phase làm các đoạn chạy dọc tuyến.
void strokeDashed(sf::VertexArray& va, const sf::Vector2f* pts, int count, float width, sf::Color color,
                  float on, float off, float phase) {
    const float period = on + off;
    float walked = 0.0f;
    for (int i = 0; i + 1 < count; ++i) {
        const sf::Vector2f d = pts[i + 1] - pts[i];
        const float length = sqrt(d.x * d.x + d.y * d.y);
        if (length < 0.001f) continue;
        const sf::Vector2f unit(d.x / length, d.y / length);
        float t = 0.0f;
        while (t < length) {
            float cursor = fmod(walked + t - phase, period);
            if (cursor < 0.0f) cursor += period;
            if (cursor < on) {
                const float step = min(on - cursor, length - t);
                appendQuad(va, pts[i] + unit * t, pts[i] + unit * (t + step), width, color);
                t += step + 0.001f;
            } else {
                t += min(period - cursor, length - t) + 0.001f;
            }
        }
        walked += length;
    }
}

// Ánh xạ tọa độ khung bản đồ sang điểm ảnh.
struct MapTransform {
    float ax;
    float ay;
    float kx;
    float ky;
    sf::Vector2f operator()(float x, float y) const { return {ax + x * kx, ay + y * ky}; }
};

// Tô vùng đất lõm bằng stencil: mỗi vòng vẽ quạt đảo bit, phần lẻ là bên trong.
void fillLayer(sf::RenderTarget& target, const MapData& map, MapLayer layer, const MapTransform& tf,
               sf::Color fill, sf::Color border, float borderWidth, sf::FloatRect cover) {
    target.clearStencil(sf::StencilValue(0));

    sf::RenderStates mark;
    mark.stencilMode.stencilComparison = sf::StencilComparison::Always;
    mark.stencilMode.stencilUpdateOperation = sf::StencilUpdateOperation::Invert;
    mark.stencilMode.stencilReference = sf::StencilValue(1);
    mark.stencilMode.stencilMask = sf::StencilValue(1);
    mark.stencilMode.stencilOnly = true;

    sf::VertexArray outline(sf::PrimitiveType::Triangles);
    for (int r = map.ringStart(layer); r < map.ringEnd(layer); ++r) {
        const MapRing& ring = map.ring(r);
        sf::VertexArray fan(sf::PrimitiveType::TriangleFan, static_cast<size_t>(ring.count));
        sf::Vector2f previous;
        for (int i = 0; i < ring.count; ++i) {
            const MapPoint& p = map.point(ring.start + i);
            const sf::Vector2f px = tf(p.x, p.y);
            fan[static_cast<size_t>(i)].position = px;
            if (i > 0 && borderWidth > 0.0f) appendQuad(outline, previous, px, borderWidth, border);
            previous = px;
        }
        target.draw(fan, mark);
    }

    sf::RenderStates paint;
    paint.stencilMode.stencilComparison = sf::StencilComparison::Equal;
    paint.stencilMode.stencilUpdateOperation = sf::StencilUpdateOperation::Keep;
    paint.stencilMode.stencilReference = sf::StencilValue(1);
    paint.stencilMode.stencilMask = sf::StencilValue(1);

    sf::RectangleShape quad(cover.size);
    quad.setPosition(cover.position);
    quad.setFillColor(fill);
    target.draw(quad, paint);
    if (borderWidth > 0.0f) target.draw(outline);
}

struct TrainMark {
    float sx, sy, cx, cy, lx, ly, ax, ay;
    bool running;
    bool dashed;
    bool flip;
    int priority;
    sf::Color color;
    string tag;
    string brief;
};

void sortByY(TrainMark** list, int count) {
    for (int i = 1; i < count; ++i) {
        TrainMark* item = list[i];
        int j = i - 1;
        while (j >= 0 && list[j]->sy > item->sy) {
            list[j + 1] = list[j];
            --j;
        }
        list[j + 1] = item;
    }
}

// Xếp các nhãn tàu thành cột, đẩy nhau ra để không chồng lấn.
void stackMarks(TrainMark** list, int count, float height, float gap, float shift) {
    sortByY(list, count);
    float y = -100000.0f;
    for (int i = 0; i < count; ++i) {
        y = max(max(list[i]->sy - height / 2.0f + shift, y + gap), 6.0f);
        list[i]->cy = y;
    }
    float floorY = STAGE_H - gap;
    for (int i = count - 1; i >= 0; --i) {
        if (list[i]->cy > floorY) list[i]->cy = floorY;
        floorY = list[i]->cy - gap;
    }
}

}

string formatMinute(double minute) {
    const int total = static_cast<int>(floor(minute));
    const int day = total / 1440;
    const int inDay = total % 1440;
    stringstream out;
    if (day > 0) out << "D+" << day << " ";
    out << setfill('0') << setw(2) << inDay / 60 << ":" << setw(2) << inDay % 60;
    return out.str();
}

string formatDuration(int minutes) {
    const int value = max(0, minutes);
    if (value < 60) return to_string(value) + " phút";
    string out = to_string(value / 60) + " giờ";
    if (value % 60) out += " " + to_string(value % 60) + " phút";
    return out;
}

// ==================== MÔ HÌNH DỮ LIỆU ====================
SimModel::SimModel() : stations(0), trains(0), windows(0), earliest(0), latest(0) {}

void SimModel::rebuild(const StationList& stationList, const TrainList& trainList,
                       const TrainList& baseline, const MapData& map) {
    stations = 0;
    for (LinkedList<StationNode*>::Node* node = stationList.getHead();
         node && stations < SIM_MAX_STATIONS; node = node->next) {
        const StationNode* st = node->data;
        SimStation row;
        row.id = st->getStationID();
        row.name = st->getName();
        row.km = st->getKmMarker();
        row.tracks = st->getMaxTracks();
        const StationMeta* meta = map.metaOf(row.id);
        row.side = meta ? meta->side : 'l';
        row.major = meta ? meta->major : false;
        int slot = stations++;
        while (slot > 0 && stationRows[slot - 1].km > row.km) {
            stationRows[slot] = stationRows[slot - 1];
            --slot;
        }
        stationRows[slot] = row;
    }

    earliest = numeric_limits<int>::max();
    latest = 0;
    trains = 0;
    for (LinkedList<TrainNode*>::Node* node = trainList.getHead();
         node && trains < SIM_MAX_TRAINS; node = node->next) {
        const TrainNode* source = node->data;
        SimTrain& row = trainRows[trains++];
        row.code = source->getTrainCode();
        row.direction = source->getDirection();
        row.priority = source->getPriority();
        row.stopCount = 0;
        const TrainNode* base = baseline.findTrainByCode(row.code);
        for (LinkedList<StopScheduleNode*>::Node* stop = source->getHeadSchedule();
             stop && row.stopCount < SIM_MAX_STOPS; stop = stop->next) {
            SimStop& out = row.stops[row.stopCount++];
            out.stationID = stop->data->getStationID();
            out.arrival = stop->data->getArrivalMinute();
            out.departure = stop->data->getDepartureMinute();
            out.track = stop->data->getTrackUsed();
            out.baseDeparture = out.departure;
            out.hasBase = false;
            if (base) {
                for (LinkedList<StopScheduleNode*>::Node* b = base->getHeadSchedule(); b; b = b->next) {
                    if (b->data->getStationID() == out.stationID) {
                        out.baseDeparture = b->data->getDepartureMinute();
                        out.hasBase = true;
                        break;
                    }
                }
            }
            earliest = min(earliest, out.arrival);
            latest = max(latest, out.departure);
        }
    }
    if (earliest == numeric_limits<int>::max()) earliest = 0;

    windows = 0;
    for (int i = 0; i < trainList.getMaintenanceCount() && windows < SIM_MAX_WINDOWS; ++i) {
        const MaintenanceWindow& source = trainList.getMaintenance(i);
        SimWindow& row = windowRows[windows++];
        row.type = source.resourceType;
        row.first = source.firstID;
        row.second = source.secondID;
        row.start = source.startMinute;
        row.end = source.endMinute;
        if (row.type == 0) row.label = "Ga " + nameOf(row.first);
        else if (row.type == 1) row.label = "Khu gian " + nameOf(row.first) + " – " + nameOf(row.second);
        else row.label = "Ray " + to_string(row.second) + " ga " + nameOf(row.first);
    }
}

const SimStation* SimModel::findStation(int id) const {
    for (int i = 0; i < stations; ++i) {
        if (stationRows[i].id == id) return &stationRows[i];
    }
    return nullptr;
}

string SimModel::nameOf(int id) const {
    const SimStation* st = findStation(id);
    return st ? st->name : "Ga " + to_string(id);
}

double SimModel::kmOf(int id) const {
    const SimStation* st = findStation(id);
    return st ? st->km : 0.0;
}

int SimModel::stopDelay(const SimTrain& train, int stop) const {
    const SimStop& s = train.stops[stop];
    return s.hasBase ? s.departure - s.baseDeparture : 0;
}

int SimModel::addedWait(const SimTrain& train, int stop) const {
    if (stop <= 0 || stop >= train.stopCount - 1) return 0;
    return max(0, stopDelay(train, stop) - stopDelay(train, stop - 1));
}

string SimModel::waitReason(int trainIndex, int stop, int reference, bool longForm) const {
    const SimTrain& train = trainRows[trainIndex];
    const SimStop& here = train.stops[stop];
    if (stop + 1 >= train.stopCount) return "Chờ lệnh điều phối";
    const SimStop& next = train.stops[stop + 1];

    for (int i = 0; i < windows; ++i) {
        const SimWindow& w = windowRows[i];
        const bool sameBlock = w.type == 1 && ((w.first == here.stationID && w.second == next.stationID) ||
                                               (w.second == here.stationID && w.first == next.stationID));
        if (sameBlock && w.end >= here.departure) {
            return (longForm ? "Bảo trì khu gian " : "Chờ bảo trì ") + nameOf(w.first) + " – " + nameOf(w.second);
        }
        if (w.type == 0 && w.first == next.stationID && w.end >= next.arrival && w.start <= next.arrival) {
            return (longForm ? "Bảo trì ga " : "Chờ bảo trì ga ") + nameOf(w.first);
        }
    }

    int bestTrain = -1;
    int bestTime = -1;
    for (int t = 0; t < trains; ++t) {
        if (t == trainIndex) continue;
        const SimTrain& other = trainRows[t];
        for (int j = 0; j + 1 < other.stopCount; ++j) {
            const SimStop& a = other.stops[j];
            const SimStop& b = other.stops[j + 1];
            const bool same = (a.stationID == here.stationID && b.stationID == next.stationID) ||
                              (a.stationID == next.stationID && b.stationID == here.stationID);
            if (same && a.departure < here.departure && b.arrival > reference &&
                b.arrival <= here.departure + 15 && b.arrival > bestTime) {
                bestTrain = t;
                bestTime = b.arrival;
            }
        }
    }
    if (bestTrain >= 0) {
        if (longForm) {
            return "Chờ " + trainRows[bestTrain].code + " rời khu gian " +
                   nameOf(here.stationID) + " – " + nameOf(next.stationID);
        }
        return "Chờ " + trainRows[bestTrain].code + " rời khu gian phía trước";
    }
    return "Chờ lệnh điều phối";
}

TrainStatus SimModel::status(int trainIndex, double minute) const {
    const SimTrain& train = trainRows[trainIndex];
    TrainStatus out;
    out.mode = TrainMode::Arrived;
    out.stop = max(0, train.stopCount - 1);
    out.km = train.stopCount ? kmOf(train.stops[out.stop].stationID) : 0.0;
    out.delay = 0;
    out.color = Palette::Muted;
    out.dashed = false;
    if (train.stopCount == 0) {
        out.mode = TrainMode::Waiting;
        out.brief = "Chưa có lịch";
        out.line1 = "Chưa có điểm dừng";
        return out;
    }

    const int n = train.stopCount;
    if (minute < train.stops[0].arrival) {
        out.mode = TrainMode::Waiting;
        out.stop = 0;
    } else if (minute <= train.stops[n - 1].departure) {
        for (int i = 0; i < n; ++i) {
            const SimStop& a = train.stops[i];
            if (minute >= a.arrival && minute <= a.departure) {
                out.stop = i;
                if (i == 0) out.mode = TrainMode::Origin;
                else if (i == n - 1) out.mode = TrainMode::Arrived;
                else out.mode = addedWait(train, i) > 0 ? TrainMode::Holding : TrainMode::Stopped;
                break;
            }
            if (i + 1 < n && minute > a.departure && minute < train.stops[i + 1].arrival) {
                out.stop = i;
                out.mode = TrainMode::Running;
                break;
            }
        }
    }

    const SimStop& stop = train.stops[out.stop];
    const string here = nameOf(stop.stationID);
    const string ray = to_string(stop.track);
    out.km = kmOf(stop.stationID);

    switch (out.mode) {
    case TrainMode::Running: {
        const SimStop& next = train.stops[out.stop + 1];
        const double t = (minute - stop.departure) / max(1, next.arrival - stop.departure);
        out.km += t * (kmOf(next.stationID) - kmOf(stop.stationID));
        out.delay = stopDelay(train, out.stop);
        out.brief = "Đang chạy";
        out.color = Palette::Blue;
        out.line1 = "Đang chạy " + here + " → " + nameOf(next.stationID);
        out.line2 = "Đến " + formatMinute(next.arrival) + " · còn " +
                    formatDuration(static_cast<int>(round(next.arrival - minute)));
        break;
    }
    case TrainMode::Holding:
        out.delay = stopDelay(train, out.stop);
        out.brief = stop.track > 1 ? "Tránh ở ray " + ray : "Chờ ở ga";
        out.color = stop.track > 1 ? Palette::Violet : Palette::Amber;
        out.line1 = waitReason(trainIndex, out.stop, static_cast<int>(minute), false);
        out.line2 = here + ", ray " + ray + " · rời " + formatMinute(stop.departure);
        break;
    case TrainMode::Stopped:
        out.delay = stopDelay(train, out.stop);
        out.brief = "Đỗ ga, ray " + ray;
        out.color = Palette::Amber;
        out.line1 = "Đỗ ở " + here + ", ray " + ray;
        out.line2 = "Rời " + formatMinute(stop.departure) + " · còn " +
                    formatDuration(static_cast<int>(round(stop.departure - minute)));
        break;
    case TrainMode::Waiting:
    case TrainMode::Origin:
        out.brief = "Chưa xuất phát";
        out.dashed = true;
        out.line1 = "Chờ xuất phát ở " + here;
        out.line2 = "Khởi hành " + formatMinute(stop.departure);
        break;
    case TrainMode::Arrived:
        out.delay = stopDelay(train, out.stop);
        out.brief = "Đã đến ga cuối";
        out.line1 = "Đã đến " + here;
        out.line2 = "Lúc " + formatMinute(stop.arrival);
        break;
    }
    return out;
}

bool SimModel::stationClosed(int id, double minute) const {
    for (int i = 0; i < windows; ++i) {
        const SimWindow& w = windowRows[i];
        if (w.type == 0 && w.first == id && minute >= w.start && minute < w.end) return true;
    }
    return false;
}

bool SimModel::blockClosed(int a, int b, double minute) const {
    for (int i = 0; i < windows; ++i) {
        const SimWindow& w = windowRows[i];
        const bool same = (w.first == a && w.second == b) || (w.first == b && w.second == a);
        if (w.type == 1 && same && minute >= w.start && minute < w.end) return true;
    }
    return false;
}

int SimModel::activeWindowCount(double minute) const {
    int count = 0;
    for (int i = 0; i < windows; ++i) {
        if (minute >= windowRows[i].start && minute < windowRows[i].end) ++count;
    }
    return count;
}

// Tìm các sự kiện gần nhất sau thời điểm minute bằng phép chọn dần.
int SimModel::upcoming(double minute, SimEvent* out, int capacity) const {
    int written = 0;
    int afterTime = static_cast<int>(floor(minute));
    int skipKey = -1;
    while (written < capacity) {
        int bestTime = numeric_limits<int>::max();
        int bestKey = -1;
        const auto consider = [&](int time, int key) {
            const bool later = time > afterTime || (time == afterTime && key > skipKey && skipKey >= 0);
            if (later && (time < bestTime || (time == bestTime && key < bestKey))) {
                bestTime = time;
                bestKey = key;
            }
        };
        for (int i = 0; i < windows; ++i) {
            consider(windowRows[i].start, i * 2);
            consider(windowRows[i].end, i * 2 + 1);
        }
        for (int t = 0; t < trains; ++t) {
            const SimTrain& train = trainRows[t];
            for (int s = 0; s < train.stopCount; ++s) {
                const int base = 100000 + t * 1000 + s * 2;
                if (s > 0) consider(train.stops[s].arrival, base);
                if (s < train.stopCount - 1) consider(train.stops[s].departure, base + 1);
            }
        }
        if (bestKey < 0) break;

        SimEvent& event = out[written++];
        event.minute = bestTime;
        if (bestKey < 100000) {
            const SimWindow& w = windowRows[bestKey / 2];
            event.text = w.label + (bestKey % 2 ? " mở lại" : " bắt đầu bảo trì");
        } else {
            const int value = bestKey - 100000;
            const SimTrain& train = trainRows[value / 1000];
            const SimStop& stop = train.stops[(value % 1000) / 2];
            event.text = train.code + (value % 2 ? " rời " : " đến ") + nameOf(stop.stationID);
        }
        afterTime = bestTime;
        skipKey = bestKey;
    }
    return written;
}

// ==================== BẢN ĐỒ ====================
GraphicalSimulationEngine::GraphicalSimulationEngine()
    : currentMinute(0.0), playing(false), speedIndex(1), zoomIndex(0), viewScale(1.0f), viewX(0.0f),
      viewY(0.0f), dragging(false), firstRow(0) {}

void GraphicalSimulationEngine::resetClock(const SimModel& model) {
    currentMinute = model.firstMinute();
    int firstWindow = numeric_limits<int>::max();
    for (int i = 0; i < model.windowCount(); ++i) firstWindow = min(firstWindow, model.window(i).start);
    if (firstWindow != numeric_limits<int>::max() && firstWindow + 60 < model.lastMinute()) {
        currentMinute = firstWindow + 60;
    }
}

void GraphicalSimulationEngine::update(Ui::Canvas& ui, const SimModel& model) {
    const double start = max(0.0, model.firstMinute() - 15.0);
    const double end = model.lastMinute() + 60.0;
    if (ui.keySpace()) playing = !playing;
    if (ui.keyLeft()) currentMinute -= 15.0;
    if (ui.keyRight()) currentMinute += 15.0;
    if (playing) {
        currentMinute += SPEED_VALUE[speedIndex] * ui.deltaTime();
        if (currentMinute >= end) playing = false;
    }
    currentMinute = min(end, max(start, currentMinute));
}

void GraphicalSimulationEngine::drawStaticMap(Ui::Canvas& ui, const MapData& map, const SimModel& model,
                                              float x, float y) {
    drawMap(ui, map, model, x, y, false);
}

void GraphicalSimulationEngine::drawMap(Ui::Canvas& ui, const MapData& map, const SimModel& model,
                                        float x, float y, bool full) {
    const double m = currentMinute;
    const float rate = min(1.0f, ui.deltaTime() * 10.0f);
    if (full) {
        viewScale += (ZOOM_SCALE[zoomIndex] - viewScale) * rate;
        viewX += (ZOOM_X[zoomIndex] - viewX) * rate;
        viewY += (ZOOM_Y[zoomIndex] - viewY) * rate;
    }
    const float s = full ? viewScale : 1.0f;
    const float ox = full ? viewX : 0.0f;
    const float oy = full ? viewY : 0.0f;
    const bool zoomed = full && zoomIndex != 0;

    ui.frame(x, y, STAGE_W + 2.0f, STAGE_H + 2.0f, 1.0f, Palette::Ink);
    const float left = x + 1.0f;
    const float top = y + 1.0f;
    ui.setScissor(left, top, STAGE_W, STAGE_H);
    ui.rect(left, top, STAGE_W, STAGE_H, Palette::Sea);

    const float px = ui.scale();
    const sf::Vector2f stageOrigin = ui.toPx(left, top);
    const MapTransform tf{stageOrigin.x - ox * s * px, stageOrigin.y - oy * s * px, s * px, s * px};
    const sf::FloatRect cover(stageOrigin, {STAGE_W * px, STAGE_H * px});
    sf::RenderTarget& target = ui.target();

    fillLayer(target, map, LayerChina, tf, Palette::Neighbor, Palette::NeighborLine, 1.0f * px, cover);
    fillLayer(target, map, LayerLaos, tf, Palette::Neighbor, Palette::NeighborLine, 1.0f * px, cover);
    fillLayer(target, map, LayerThailand, tf, Palette::Neighbor, Palette::NeighborLine, 1.0f * px, cover);
    fillLayer(target, map, LayerCambodia, tf, Palette::Neighbor, Palette::NeighborLine, 1.0f * px, cover);
    fillLayer(target, map, LayerVietnam, tf, Palette::White, Palette::Ink, 1.2f * px, cover);

    const auto stageX = [&](float bx) { return left + (bx - ox) * s; };
    const auto stageY = [&](float by) { return top + (by - oy) * s; };

    for (int i = 0; i < map.islandCount(); ++i) {
        const MapPoint& p = map.island(i);
        ui.circle(stageX(p.x), stageY(p.y), 2.2f, Palette::White, 1.0f, Palette::Ink);
    }
    if (!zoomed && s < 1.5f) {
        ui.text(left + 430.0f, top + 268.0f, "QĐ. Hoàng Sa", FontSemi, 13.0f, Palette::Ink);
        ui.text(left + 450.0f, top + 532.0f, "QĐ. Trường Sa", FontSemi, 13.0f, Palette::Ink, Align::Right);
        if (full) {
            ui.text(left + 130.0f, top + 182.0f, "LÀO", FontRegular, 13.0f, Palette::Muted);
            ui.text(left + 62.0f, top + 288.0f, "THÁI LAN", FontRegular, 13.0f, Palette::Muted);
            ui.text(left + 186.0f, top + 390.0f, "CAMPUCHIA", FontRegular, 13.0f, Palette::Muted);
            ui.text(left + 420.0f, top + 352.0f, "Biển Đông", FontRegular, 15.0f, sf::Color(61, 92, 114));
        }
    }

    sf::VertexArray lines(sf::PrimitiveType::Triangles);
    sf::Vector2f pixels[MapData::MAX_ROUTE + 2];
    MapPoint routePoints[MapData::MAX_ROUTE + 2];
    const auto toPixels = [&](const MapPoint* source, int count) {
        for (int i = 0; i < count; ++i) pixels[i] = tf(source[i].x, source[i].y);
    };

    for (int c = 0; c < map.contextCount(); ++c) {
        const ContextLine& line = map.context(c);
        const int count = min(line.count, MapData::MAX_ROUTE);
        for (int i = 0; i < count; ++i) {
            const MapPoint& p = map.point(line.start + i);
            pixels[i] = tf(p.x, p.y);
        }
        strokeLine(lines, pixels, count, 3.0f * px, Palette::Stone);
        if (full) strokeDashed(lines, pixels, count, 1.4f * px, Palette::White, 5.0f * px, 5.0f * px, 0.0f);
    }

    // Khoảng lý trình đang có tàu chạy để tô xanh các khu gian bận.
    double busyLow[SIM_MAX_TRAINS];
    double busyHigh[SIM_MAX_TRAINS];
    int busyCount = 0;
    TrainStatus statuses[SIM_MAX_TRAINS];
    for (int t = 0; t < model.trainCount(); ++t) {
        statuses[t] = model.status(t, m);
        if (statuses[t].mode == TrainMode::Running) {
            const SimTrain& train = model.train(t);
            const double a = model.kmOf(train.stops[statuses[t].stop].stationID);
            const double b = model.kmOf(train.stops[statuses[t].stop + 1].stationID);
            busyLow[busyCount] = min(a, b);
            busyHigh[busyCount] = max(a, b);
            ++busyCount;
        }
    }

    const float phase = ui.clock() * 36.7f * px;
    const auto hazardLine = [&](int count) {
        strokeLine(lines, pixels, count, 10.0f * px, Palette::Ink);
        strokeLine(lines, pixels, count, 7.5f * px, Palette::Hazard);
        strokeDashed(lines, pixels, count, 7.5f * px, Palette::WaitRed, 11.0f * px, 11.0f * px, phase);
    };

    for (int i = 0; i + 1 < model.stationCount(); ++i) {
        const SimStation& a = model.station(i);
        const SimStation& b = model.station(i + 1);
        const int count = map.routeBetween(a.km, b.km, routePoints, MapData::MAX_ROUTE);
        toPixels(routePoints, count);
        bool busy = false;
        for (int k = 0; k < busyCount; ++k) {
            if (busyLow[k] < b.km && busyHigh[k] > a.km) busy = true;
        }
        if (model.blockClosed(a.id, b.id, m)) {
            hazardLine(count);
        } else if (busy) {
            strokeLine(lines, pixels, count, 6.0f * px, Palette::Blue);
            strokeDashed(lines, pixels, count, 2.5f * px, Palette::White, 7.0f * px, 7.0f * px, 0.0f);
        } else {
            strokeLine(lines, pixels, count, 5.0f * px, Palette::Ink);
            strokeDashed(lines, pixels, count, 2.5f * px, Palette::White, 7.0f * px, 7.0f * px, 0.0f);
        }
    }
    for (int i = 0; i < model.stationCount(); ++i) {
        const SimStation& st = model.station(i);
        if (model.stationClosed(st.id, m)) {
            const int count = map.routeBetween(st.km - 14.0, st.km + 14.0, routePoints, MapData::MAX_ROUTE);
            toPixels(routePoints, count);
            hazardLine(count);
        }
    }
    target.draw(lines);

    if (zoomed) {
        // Ghi "1 ray" trên các khu gian đủ dài: toàn tuyến là đường đơn.
        for (int i = 0; i + 1 < model.stationCount(); ++i) {
            const SimStation& a = model.station(i);
            const SimStation& b = model.station(i + 1);
            if ((b.km - a.km) * 0.31 * s < 85.0) continue;
            const MapPoint mid = map.at((a.km + b.km) / 2.0);
            const float tx = stageX(mid.x) + 12.0f;
            const float ty = stageY(mid.y);
            if (tx < left - 24.0f || tx > left + STAGE_W + 24.0f || ty < top - 24.0f || ty > top + STAGE_H + 24.0f) continue;
            const float w = ui.textWidth("1 ray", FontSemi, 13.0f) + 10.0f;
            ui.rect(tx, ty - 9.0f, w, 18.0f, Palette::White);
            ui.frame(tx, ty - 9.0f, w, 18.0f, 1.0f, Palette::Ink);
            ui.text(tx + 5.0f, ty, "1 ray", FontSemi, 13.0f, Palette::Ink);
        }
    }

    float placedX[MapData::MAX_CONTEXT];
    float placedY[MapData::MAX_CONTEXT];
    int placed = 0;
    for (int c = 0; c < map.contextCount(); ++c) {
        const ContextLine& line = map.context(c);
        if (line.name.empty()) continue;
        const float lx = stageX(line.labelX);
        float ly = stageY(line.labelY);
        if (lx < left - 24.0f || lx > left + STAGE_W + 24.0f || ly < top - 24.0f || ly > top + STAGE_H + 24.0f) continue;
        // Hai nhãn tuyến nằm sát nhau thì đẩy nhãn sau xuống dòng dưới.
        for (int p = 0; p < placed; ++p) {
            if (fabs(placedX[p] - lx) < 60.0f && fabs(placedY[p] - ly) < 15.0f) ly = placedY[p] + 15.0f;
        }
        placedX[placed] = lx;
        placedY[placed] = ly;
        ++placed;
        const Align align = line.side == 'l' ? Align::Right : Align::Left;
        ui.text(lx + (line.side == 'l' ? -6.0f : 6.0f), ly, line.name, FontRegular, 13.0f, Palette::Muted, align);
    }

    // Vị trí tàu và nhãn của tàu.
    TrainMark marks[SIM_MAX_TRAINS];
    TrainMark* rightLane[SIM_MAX_TRAINS];
    TrainMark* leftLane[SIM_MAX_TRAINS];
    int markCount = 0;
    int rightCount = 0;
    int leftCount = 0;
    const float callW = full ? 132.0f : 100.0f;
    const float callH = full ? 46.0f : 28.0f;
    const float callGap = full ? 52.0f : 34.0f;
    for (int t = 0; t < model.trainCount(); ++t) {
        if (model.train(t).stopCount == 0) continue;
        const MapPoint p = map.at(statuses[t].km);
        const float sx = (p.x - ox) * s;
        const float sy = (p.y - oy) * s;
        if (sx < -24.0f || sx > STAGE_W + 24.0f || sy < -24.0f || sy > STAGE_H + 24.0f) continue;
        TrainMark& mark = marks[markCount++];
        const SimTrain& train = model.train(t);
        mark.sx = sx;
        mark.sy = sy;
        mark.running = statuses[t].mode == TrainMode::Running;
        mark.dashed = statuses[t].dashed;
        mark.flip = train.direction != 1;
        mark.priority = train.priority;
        mark.color = full ? statuses[t].color : (mark.running ? Palette::Blue : Palette::Amber);
        mark.tag = train.code + (train.direction == 1 ? " ↓" : " ↑");
        mark.brief = statuses[t].brief;
        if (sx < 375.0f) rightLane[rightCount++] = &mark;
        else leftLane[leftCount++] = &mark;
    }
    // Khi phóng to, nhãn tàu nâng lên để không đè tên ga.
    stackMarks(rightLane, rightCount, callH, callGap, zoomed ? -62.0f : 0.0f);
    stackMarks(leftLane, leftCount, callH, callGap, zoomed ? -62.0f : 0.0f);
    for (int i = 0; i < rightCount; ++i) {
        rightLane[i]->cx = full ? 412.0f : 430.0f;
        rightLane[i]->lx = rightLane[i]->cx;
        rightLane[i]->ly = rightLane[i]->cy + callH / 2.0f;
    }
    for (int i = 0; i < leftCount; ++i) {
        leftLane[i]->cx = max(8.0f, leftLane[i]->sx - (full ? 190.0f : 150.0f));
        leftLane[i]->lx = leftLane[i]->cx + callW;
        leftLane[i]->ly = leftLane[i]->cy + callH / 2.0f;
    }
    for (int i = 0; i < markCount; ++i) {
        TrainMark& mark = marks[i];
        const float dx = mark.lx - mark.sx;
        const float dy = mark.ly - mark.sy;
        const float length = max(1.0f, sqrt(dx * dx + dy * dy));
        const float inset = (full && !mark.running) ? (zoomed ? 17.0f : 13.0f) : 0.0f;
        mark.ax = mark.sx + dx / length * inset;
        mark.ay = mark.sy + dy / length * inset;
        ui.line(left + mark.ax, top + mark.ay, left + mark.lx, top + mark.ly, 1.5f, mark.color);
    }

    // Ga: vòng tròn to theo số ray, số ray ghi bên trong.
    for (int i = 0; i < model.stationCount(); ++i) {
        const SimStation& st = model.station(i);
        const MapPoint p = map.at(st.km);
        float sx = (p.x - ox) * s;
        float sy = (p.y - oy) * s;
        if (sx < -24.0f || sx > STAGE_W + 24.0f || sy < -24.0f || sy > STAGE_H + 24.0f) continue;

        const bool closed = model.stationClosed(st.id, m);
        const bool numbered = zoomed || st.major || closed;
        const float t = static_cast<float>(st.tracks);
        const float d = zoomed ? min(32.0f, round(22.0f + t * 0.9f))
                               : (numbered ? min(25.0f, round(18.0f + t * 0.6f)) : 11.0f);

        // Khu Biên Hòa, Dĩ An quá sát nhau khi xem Miền Nam nên kéo huy hiệu ra ngoài.
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        char side = st.side;
        if (zoomed) {
            side = (st.side == 'r' || sx < 240.0f) ? 'r' : 'l';
            if (zoomIndex == 3 && st.id == 116) { offsetX = 20.0f; offsetY = -32.0f; side = 'r'; }
            if (zoomIndex == 3 && st.id == 117) { offsetX = 40.0f; offsetY = 18.0f; side = 'r'; }
            if (zoomIndex == 3 && st.id == 118) side = 'b';
        }
        const float trueX = left + sx;
        const float trueY = top + sy;
        const float cx = trueX + offsetX;
        const float cy = trueY + offsetY;
        if (offsetX != 0.0f || offsetY != 0.0f) ui.line(trueX, trueY, cx, cy, 1.5f, Palette::Ink);

        // Ray đang có tàu, dùng để đổi màu viền ga và các vạch ray.
        sf::Color rayColor[64];
        bool rayUsed[64] = {false};
        bool anyUsed = false;
        sf::Color edge = Palette::Ink;
        if (full) {
            for (int k = 0; k < model.trainCount(); ++k) {
                if (statuses[k].mode == TrainMode::Running) continue;
                const SimStop& stop = model.train(k).stops[statuses[k].stop];
                if (stop.stationID == st.id && stop.track >= 1 && stop.track <= 64) {
                    rayUsed[stop.track - 1] = true;
                    rayColor[stop.track - 1] = statuses[k].color;
                    edge = statuses[k].color;
                    anyUsed = true;
                }
            }
        }

        if (closed) {
            const sf::Vector2f center = ui.toPx(cx, cy);
            const float radius = (d + 12.0f) / 2.0f * px;
            const float spin = ui.clock() * 1.26f;
            sf::VertexArray ring(sf::PrimitiveType::Triangles);
            for (int w = 0; w < 8; ++w) {
                const sf::Color color = w % 2 == 0 ? Palette::Hazard : Palette::WaitRed;
                for (int step = 0; step < 6; ++step) {
                    const float a0 = spin + (w + step / 6.0f) * PI_F / 4.0f;
                    const float a1 = spin + (w + (step + 1) / 6.0f) * PI_F / 4.0f;
                    ring.append(sf::Vertex{center, color});
                    ring.append(sf::Vertex{center + sf::Vector2f(cos(a0) * radius, sin(a0) * radius), color});
                    ring.append(sf::Vertex{center + sf::Vector2f(cos(a1) * radius, sin(a1) * radius), color});
                }
            }
            target.draw(ring);
        }

        sf::Color fill = Palette::White;
        sf::Color numberColor = Palette::Ink;
        if (numbered && st.tracks >= 6) { fill = Palette::Ink; numberColor = Palette::White; }
        else if (numbered && st.tracks >= 3) fill = Palette::Mid;
        ui.circle(cx, cy, d / 2.0f, fill, anyUsed ? 3.5f : 2.5f, edge);
        if (numbered) {
            ui.text(cx, cy, to_string(st.tracks), FontCondBold, zoomed ? 18.0f : 15.0f, numberColor, Align::Center);
        }

        if (numbered) {
            const string label = zoomed ? st.name + " · km " + to_string(static_cast<int>(st.km)) : st.name;
            const float nameWidth = ui.textWidth(label, FontSemi, 15.0f);
            const string rays = to_string(st.tracks) + " ray";
            const int ticks = min(st.tracks, 16);
            const float ticksWidth = zoomed ? ticks * 6.0f : 0.0f;
            const float raysWidth = zoomed ? ui.textWidth(rays, FontSemi, 14.0f) + 6.0f : 0.0f;
            const float content = nameWidth + 8.0f + (zoomed ? 6.0f + ticksWidth + raysWidth : 0.0f);
            const float reach = d / 2.0f + 5.0f;
            float lx = cx - reach - content;
            float ly = cy;
            if (side == 'r') lx = cx + reach;
            else if (side == 't') { lx = cx - content * 0.3f; ly = cy - reach - 9.0f; }
            else if (side == 'b') { lx = cx - d / 2.0f; ly = cy + reach + 9.0f; }
            ui.rect(lx, ly - 9.5f, content, 19.0f, sf::Color(255, 255, 255, 219));
            const sf::Color nameColor = closed ? Palette::Red : Palette::Ink;
            ui.text(lx + 4.0f, ly, label, FontSemi, 15.0f, nameColor);
            if (zoomed) {
                float tickX = lx + 4.0f + nameWidth + 6.0f;
                for (int k = 0; k < ticks; ++k) {
                    const bool used = k < 64 && rayUsed[k];
                    const float h = used ? 15.0f : 11.0f;
                    const sf::Color color = closed ? Palette::Red : (used ? rayColor[k] : Palette::Ink);
                    ui.rect(tickX, ly + 7.5f - h, 4.0f, h, color);
                    tickX += 6.0f;
                }
                ui.text(tickX + 4.0f, ly, rays, FontSemi, 14.0f, nameColor);
            }
        }
    }

    // Chấm vị trí và nhãn tàu nằm trên cùng.
    for (int i = 0; i < markCount; ++i) {
        TrainMark& mark = marks[i];
        if (mark.running) {
            if (full) {
                const float wave = fmod(ui.clock(), 1.4f) / 1.4f;
                sf::Color ping = mark.color;
                ping.a = static_cast<uint8_t>(150.0f * (1.0f - wave));
                ui.circle(left + mark.sx, top + mark.sy, 7.0f + 12.0f * wave, ping);
            }
            ui.circle(left + mark.sx, top + mark.sy, 7.0f, mark.color, 2.0f, Palette::White);
        }
        const float bx = left + mark.cx;
        const float by = top + mark.cy;
        ui.rect(bx, by, callW, callH, Palette::White);
        if (mark.dashed) {
            for (float dash = 0.0f; dash < callW; dash += 8.0f) {
                ui.rect(bx + dash, by, 4.0f, 2.0f, mark.color);
                ui.rect(bx + dash, by + callH - 2.0f, 4.0f, 2.0f, mark.color);
            }
            for (float dash = 0.0f; dash < callH; dash += 8.0f) {
                ui.rect(bx, by + dash, 2.0f, 4.0f, mark.color);
                ui.rect(bx + callW - 2.0f, by + dash, 2.0f, 4.0f, mark.color);
            }
        } else {
            ui.frame(bx, by, callW, callH, 2.0f, mark.color);
        }
        if (full) {
            ui.trainSprite(bx + 7.0f, by + 7.0f, 60.0f, 16.0f, mark.priority, mark.flip);
            ui.text(bx + 73.0f, by + 15.0f, mark.tag, FontCondBold, 19.0f, Palette::Ink);
            ui.text(bx + 7.0f, by + 34.0f, mark.brief, FontSemi, 13.0f, mark.color);
        } else {
            ui.trainSprite(bx + 6.0f, by + 8.0f, 44.0f, 12.0f, mark.priority, mark.flip);
            ui.text(bx + 55.0f, by + callH / 2.0f, mark.tag, FontCondBold, 17.0f, Palette::Ink);
        }
    }

    if (zoomed) {
        // Bản đồ nhỏ cho biết vùng đang xem nằm ở đâu trên toàn quốc.
        const float insetX = left + 8.0f;
        const float insetY = top + 8.0f;
        const float k = 76.0f / 268.0f;
        ui.rect(insetX, insetY, 78.0f, 155.0f, Palette::Sea);
        const sf::Vector2f origin = ui.toPx(insetX + 1.0f, insetY + 1.0f);
        const MapTransform mini{origin.x - 115.0f * k * px, origin.y, k * px, k * px};
        const sf::Vector2f coverOrigin = ui.toPx(insetX, insetY);
        fillLayer(target, map, LayerVietnam, mini, Palette::White, Palette::Ink, 0.8f * px,
                  sf::FloatRect(coverOrigin, {78.0f * px, 155.0f * px}));
        ui.frame(insetX, insetY, 78.0f, 155.0f, 1.0f, Palette::Ink);
        ui.frame(insetX + 1.0f + (ZOOM_X[zoomIndex] - 115.0f) * k,
                 insetY + 1.0f + ZOOM_Y[zoomIndex] * k,
                 STAGE_W / ZOOM_SCALE[zoomIndex] * k, STAGE_H / ZOOM_SCALE[zoomIndex] * k,
                 1.5f, Palette::WaitRed);
    }

    // Chú giải.
    const int legendRows = full ? 6 : 3;
    const float legendW = full ? 164.0f : 132.0f;
    const float legendH = 12.0f + legendRows * 20.0f;
    const float legendX = left + 8.0f;
    const float legendY = top + STAGE_H - 8.0f - legendH;
    ui.rect(legendX, legendY, legendW, legendH, Palette::White);
    ui.frame(legendX, legendY, legendW, legendH, 1.0f, Palette::Ink);
    float rowY = legendY + 16.0f;
    const float swatch = legendX + 8.0f;
    const float label = legendX + 46.0f;
    const auto railSwatch = [&](float width, sf::Color color) {
        for (float dash = 0.0f; dash < 30.0f; dash += 8.0f) ui.rect(swatch + dash, rowY - width / 2.0f, 6.0f, width, color);
    };
    if (full) {
        railSwatch(5.0f, Palette::Ink);
        ui.text(label, rowY, "Đường trống", FontRegular, 13.0f, Palette::Ink);
        rowY += 20.0f;
        railSwatch(6.0f, Palette::Blue);
        ui.text(label, rowY, "Đang có tàu", FontRegular, 13.0f, Palette::Ink);
        rowY += 20.0f;
    } else {
        ui.text(legendX + 8.0f, rowY, "Lúc " + formatMinute(m), FontSemi, 13.0f, Palette::Ink);
        rowY += 20.0f;
    }
    ui.hazard(swatch, rowY - 4.0f, 30.0f, 8.0f);
    ui.text(label, rowY, "Đang bảo trì", FontRegular, 13.0f, Palette::Ink);
    rowY += 20.0f;
    if (full) {
        for (float dash = 0.0f; dash < 30.0f; dash += 7.0f) ui.rect(swatch + dash, rowY - 1.5f, 5.0f, 3.0f, Palette::Stone);
        ui.text(label, rowY, "Tuyến khác", FontRegular, 13.0f, Palette::Ink);
        rowY += 20.0f;
    }
    ui.circle(swatch + 15.0f, rowY, 9.0f, Palette::Ink);
    ui.text(swatch + 15.0f, rowY, "6", FontCondBold, 14.0f, Palette::White, Align::Center);
    ui.text(label, rowY, "Số ray tại ga", FontRegular, 13.0f, Palette::Ink);
    rowY += 20.0f;
    if (full) {
        ui.rect(swatch + 8.0f, rowY - 7.5f, 4.0f, 15.0f, Palette::Blue);
        ui.rect(swatch + 14.0f, rowY - 3.5f, 4.0f, 11.0f, Palette::Ink);
        ui.rect(swatch + 20.0f, rowY - 3.5f, 4.0f, 11.0f, Palette::Ink);
        ui.text(label, rowY, "Ray có tàu", FontRegular, 13.0f, Palette::Ink);
    }

    ui.text(left + STAGE_W - 6.0f, top + STAGE_H - 9.0f, "Natural Earth · OpenStreetMap", FontRegular, 11.0f,
            sf::Color(51, 71, 90), Align::Right);

    ui.clearScissor();
}

// ==================== MÀN BẢN ĐỒ MÔ PHỎNG ====================
void GraphicalSimulationEngine::drawControls(Ui::Canvas& ui, const SimModel& model) {
    const float rowY = 78.0f;
    const float mid = rowY + 22.0f;
    if (ui.button(1001, 32.0f, rowY, 104.0f, 44.0f, playing ? "Tạm dừng" : "Chạy", ButtonStyle::Solid)) {
        if (!playing && currentMinute >= model.lastMinute() + 60.0) currentMinute = max(0.0, model.firstMinute() - 15.0);
        playing = !playing;
    }
    const int total = static_cast<int>(floor(currentMinute));
    const float clockW = ui.text(148.0f, mid, formatMinute(total % 1440), FontCondSemi, 34.0f, Palette::Ink);
    ui.text(154.0f + clockW, mid + 4.0f, "Ngày D+" + to_string(total / 1440), FontRegular, 15.0f, Palette::Muted);

    const float sliderX = 292.0f;
    const float sliderW = 678.0f;
    const double start = max(0.0, model.firstMinute() - 15.0);
    const double end = model.lastMinute() + 60.0;
    const auto toX = [&](double value) { return sliderX + static_cast<float>((value - start) / (end - start)) * sliderW; };
    for (int i = 0; i < model.windowCount(); ++i) {
        const SimWindow& w = model.window(i);
        ui.hazard(toX(w.start), rowY + 8.0f, max(3.0f, toX(w.end) - toX(w.start)), 6.0f);
    }
    ui.rect(sliderX, mid - 2.0f, sliderW, 4.0f, Palette::Rule);
    const float handle = toX(currentMinute);
    ui.rect(sliderX, mid - 2.0f, handle - sliderX, 4.0f, Palette::Ink);
    const bool over = ui.hovered(sliderX - 8.0f, rowY, sliderW + 16.0f, 44.0f);
    if (over) ui.markHand();
    if (ui.clicked(sliderX - 8.0f, rowY, sliderW + 16.0f, 44.0f)) dragging = true;
    if (!ui.mouseDown()) dragging = false;
    if (dragging) {
        const float ratio = max(0.0f, min(1.0f, (ui.mouse().x - sliderX) / sliderW));
        currentMinute = start + ratio * (end - start);
    }
    const float grow = ui.anim(1002, (over || dragging) ? 1.0f : 0.0f);
    ui.circle(handle, mid, 8.0f + 2.0f * grow, Palette::Ink, 2.0f, Palette::White);

    const float speedW = 62.0f;
    const float speedX = 986.0f;
    for (int i = 0; i < 3; ++i) {
        if (ui.button(1010 + i, speedX + i * speedW, rowY, speedW, 44.0f, SPEED_LABEL[i],
                      ButtonStyle::Segment, speedIndex == i)) speedIndex = i;
    }
    if (ui.button(1020, 1180.0f, rowY, 70.0f, 44.0f, "−1 giờ", ButtonStyle::Outline)) currentMinute -= 60.0;
    if (ui.button(1021, 1258.0f, rowY, 70.0f, 44.0f, "+1 giờ", ButtonStyle::Outline)) currentMinute += 60.0;
}

void GraphicalSimulationEngine::drawPanel(Ui::Canvas& ui, const SimModel& model) {
    const double m = currentMinute;
    const float panelX = 618.0f;
    const float panelW = 710.0f;
    const float top = 134.0f;

    int running = 0;
    int stopped = 0;
    int late = 0;
    TrainStatus statuses[SIM_MAX_TRAINS];
    for (int t = 0; t < model.trainCount(); ++t) {
        statuses[t] = model.status(t, m);
        if (statuses[t].mode == TrainMode::Running) ++running;
        if (statuses[t].mode == TrainMode::Stopped || statuses[t].mode == TrainMode::Holding) ++stopped;
        if (statuses[t].delay > 0) ++late;
    }
    const int active = model.activeWindowCount(m);
    const int values[4] = {running, stopped, late, active};
    const char* const labels[4] = {"đang chạy", "đang dừng", "tàu trễ giờ", "đoạn bảo trì"};
    const sf::Color colors[4] = {Palette::Blue, Palette::Ink, late ? Palette::Red : Palette::Ink,
                                 active ? Palette::Red : Palette::Ink};
    const float columnW = panelW / 4.0f;
    for (int i = 0; i < 4; ++i) {
        const float cx = panelX + i * columnW;
        ui.rect(cx, top, 1.0f, 62.0f, Palette::Rule);
        ui.text(cx + 10.0f, top + 20.0f, to_string(values[i]), FontCondSemi, 36.0f, colors[i]);
        ui.text(cx + 10.0f, top + 48.0f, labels[i], FontRegular, 15.0f, Palette::Muted);
    }
    ui.rect(panelX, top + 62.0f, panelW, 1.0f, Palette::Rule);

    const float rowsTop = top + 63.0f;
    const float rowH = 76.0f;
    const int visible = min(5, model.trainCount());
    if (ui.hovered(panelX, rowsTop, panelW, rowH * visible)) firstRow -= static_cast<int>(ui.wheel());
    firstRow = max(0, min(firstRow, model.trainCount() - visible));

    for (int v = 0; v < visible; ++v) {
        const int t = firstRow + v;
        const SimTrain& train = model.train(t);
        const TrainStatus& status = statuses[t];
        const float y = rowsTop + v * rowH;
        ui.trainSprite(panelX, y + 14.0f, 104.0f, 28.0f, train.priority, train.direction != 1);
        ui.rect(panelX, y + 50.0f, 104.0f, 4.0f, Palette::Rule);
        if (train.stopCount > 1) {
            const double first = model.kmOf(train.stops[0].stationID);
            const double last = model.kmOf(train.stops[train.stopCount - 1].stationID);
            const double span = abs(last - first);
            const float ratio = span > 0.0 ? static_cast<float>(abs(status.km - first) / span) : 0.0f;
            ui.rect(panelX, y + 50.0f, 104.0f * ratio, 4.0f, Palette::Ink);
        }

        const float textX = panelX + 124.0f;
        const float codeW = ui.text(textX, y + 18.0f, train.code, FontCondBold, 24.0f, Palette::Ink);
        const string meta = string(train.direction == 1 ? "→ Sài Gòn" : "→ Hà Nội") +
                            " · km " + to_string(static_cast<int>(round(status.km)));
        ui.text(textX + codeW + 8.0f, y + 18.0f, meta, FontRegular, 15.0f, Palette::Muted);
        const float textW = panelW - 124.0f - 130.0f;
        ui.textFit(textX, y + 41.0f, status.line1, FontSemi, 15.0f, status.color, textW);
        ui.textFit(textX, y + 61.0f, status.line2, FontRegular, 15.0f, Palette::Body, textW);

        const string delay = status.delay > 0 ? "Trễ " + formatDuration(status.delay) : "Đúng giờ";
        ui.text(panelX + panelW, y + 38.0f, delay, FontSemi, 15.0f,
                status.delay > 0 ? Palette::Red : Palette::Green, Align::Right);
        ui.rect(panelX, y + rowH - 1.0f, panelW, 1.0f, Palette::Rule);
    }

    float y = rowsTop + visible * rowH + 16.0f;
    ui.text(panelX, y, "Sắp xảy ra", FontCondSemi, 20.0f, Palette::Ink);
    y += 24.0f;
    SimEvent events[4];
    const int count = model.upcoming(m, events, 4);
    for (int i = 0; i < count; ++i) {
        ui.text(panelX, y, formatMinute(events[i].minute), FontSemi, 15.0f, Palette::Ink);
        ui.text(panelX + 96.0f, y, events[i].text, FontRegular, 15.0f, Palette::Ink);
        y += 24.0f;
    }
}

bool GraphicalSimulationEngine::drawScreen(Ui::Canvas& ui, const MapData& map, const SimModel& model) {
    drawControls(ui, model);

    const float buttonW = 90.0f;
    for (int i = 0; i < 4; ++i) {
        if (ui.button(1030 + i, 32.0f + i * buttonW, 134.0f, buttonW, 44.0f, ZOOM_LABEL[i],
                      ButtonStyle::Segment, zoomIndex == i)) zoomIndex = i;
    }
    ui.button(1040, 454.0f, 134.0f, 70.0f, 44.0f, "Bản đồ", ButtonStyle::Segment, true);
    const bool openChart = ui.button(1041, 524.0f, 134.0f, 70.0f, 44.0f, "Đồ thị", ButtonStyle::Segment, false);

    drawMap(ui, map, model, 32.0f, 182.0f, true);
    drawPanel(ui, model);
    return openChart;
}
