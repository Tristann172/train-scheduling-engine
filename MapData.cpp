#include "MapData.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

using namespace std;

MapData::MapData()
    : pointCount(0), ringCount(0), routeCount(0), metaCount(0), contexts(0), islands(0) {
    for (int i = 0; i < LAYER_COUNT; ++i) {
        layerFirst[i] = 0;
        layerCount[i] = 0;
    }
}

static char sideCode(const string& side) {
    if (side == "right") return 'r';
    if (side == "top") return 't';
    if (side == "below") return 'b';
    return 'l';
}

// Đọc đường viền dạng "M x yL x yZ", mỗi cụm M..Z là một vòng khép kín.
void MapData::parseLand(MapLayer layer, const string& path) {
    layerFirst[layer] = ringCount;
    const char* cursor = path.c_str();
    while (*cursor) {
        if (*cursor == 'M' || *cursor == 'L') {
            const bool startRing = *cursor == 'M';
            ++cursor;
            char* end = nullptr;
            const float x = strtof(cursor, &end);
            cursor = end;
            const float y = strtof(cursor, &end);
            cursor = end;
            if (pointCount >= MAX_POINTS) return;
            if (startRing) {
                if (ringCount >= MAX_RINGS) return;
                rings[ringCount++] = {pointCount, 0};
                ++layerCount[layer];
            }
            points[pointCount++] = {x, y};
            ++rings[ringCount - 1].count;
        } else {
            ++cursor;
        }
    }
}

void MapData::parseContext(const string& name, char side, float x, float y, const string& pts) {
    if (contexts >= MAX_CONTEXT) return;
    ContextLine& item = contextLines[contexts++];
    item.name = name;
    item.side = side;
    item.labelX = x;
    item.labelY = y;
    item.start = pointCount;
    item.count = 0;
    const char* cursor = pts.c_str();
    while (*cursor && pointCount < MAX_POINTS) {
        char* end = nullptr;
        const float px = strtof(cursor, &end);
        if (end == cursor) break;
        cursor = end;
        if (*cursor == ',') ++cursor;
        const float py = strtof(cursor, &end);
        cursor = end;
        while (*cursor == ' ') ++cursor;
        points[pointCount++] = {px, py};
        ++item.count;
    }
}

bool MapData::load(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) return false;

    string line;
    while (getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;

        string fields[6];
        int count = 0;
        stringstream parser(line);
        while (count < 6 && getline(parser, fields[count], '|')) ++count;

        if (fields[0] == "LAND" && count >= 3) {
            MapLayer layer = LayerVietnam;
            if (fields[1] == "chn") layer = LayerChina;
            else if (fields[1] == "lao") layer = LayerLaos;
            else if (fields[1] == "tha") layer = LayerThailand;
            else if (fields[1] == "khm") layer = LayerCambodia;
            parseLand(layer, fields[2]);
        } else if (fields[0] == "ROUTE" && count >= 4 && routeCount < MAX_ROUTE) {
            route[routeCount++] = {atof(fields[1].c_str()),
                                   static_cast<float>(atof(fields[2].c_str())),
                                   static_cast<float>(atof(fields[3].c_str()))};
        } else if (fields[0] == "STATION" && count >= 4 && metaCount < MAX_META) {
            meta[metaCount++] = {atoi(fields[1].c_str()), sideCode(fields[2]), fields[3] == "1"};
        } else if (fields[0] == "CONTEXT" && count >= 6) {
            parseContext(fields[1], sideCode(fields[2]),
                         static_cast<float>(atof(fields[3].c_str())),
                         static_cast<float>(atof(fields[4].c_str())), fields[5]);
        } else if (fields[0] == "ISLAND" && count >= 3 && islands < MAX_ISLANDS) {
            islandPoints[islands++] = {static_cast<float>(atof(fields[1].c_str())),
                                       static_cast<float>(atof(fields[2].c_str()))};
        }
    }
    return routeCount >= 2;
}

MapPoint MapData::at(double km) const {
    if (routeCount == 0) return {0.0f, 0.0f};
    if (km <= route[0].km) return {route[0].x, route[0].y};
    for (int i = 1; i < routeCount; ++i) {
        if (km <= route[i].km) {
            const RoutePoint& a = route[i - 1];
            const RoutePoint& b = route[i];
            const double span = b.km - a.km;
            const float t = span > 0.0 ? static_cast<float>((km - a.km) / span) : 0.0f;
            return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
        }
    }
    return {route[routeCount - 1].x, route[routeCount - 1].y};
}

int MapData::routeBetween(double k0, double k1, MapPoint* out, int capacity) const {
    if (capacity < 2) return 0;
    int written = 0;
    out[written++] = at(k0);
    for (int i = 0; i < routeCount && written < capacity - 1; ++i) {
        if (route[i].km > k0 && route[i].km < k1) out[written++] = {route[i].x, route[i].y};
    }
    out[written++] = at(k1);
    return written;
}

const StationMeta* MapData::metaOf(int stationID) const {
    for (int i = 0; i < metaCount; ++i) {
        if (meta[i].id == stationID) return &meta[i];
    }
    return nullptr;
}
