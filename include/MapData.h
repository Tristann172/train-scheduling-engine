#pragma once

#include <string>

// Dữ liệu bản đồ đọc từ data/map_data.txt, toàn bộ lưu trong mảng tĩnh.
struct MapPoint {
    float x;
    float y;
};

enum MapLayer { LayerChina, LayerLaos, LayerThailand, LayerCambodia, LayerVietnam, LAYER_COUNT };

struct MapRing {
    int start;
    int count;
};

struct RoutePoint {
    double km;
    float x;
    float y;
};

struct StationMeta {
    int id;
    char side;   // 'l' trái, 'r' phải, 't' trên, 'b' dưới
    bool major;
};

struct ContextLine {
    std::string name;
    char side;
    float labelX;
    float labelY;
    int start;
    int count;
};

class MapData {
public:
    inline static constexpr int MAX_POINTS = 12000;
    inline static constexpr int MAX_RINGS = 160;
    inline static constexpr int MAX_ROUTE = 640;
    inline static constexpr int MAX_META = 128;
    inline static constexpr int MAX_CONTEXT = 16;
    inline static constexpr int MAX_ISLANDS = 32;

    MapData();
    bool load(const std::string& filename);

    int ringStart(MapLayer layer) const { return layerFirst[layer]; }
    int ringEnd(MapLayer layer) const { return layerFirst[layer] + layerCount[layer]; }
    const MapRing& ring(int index) const { return rings[index]; }
    const MapPoint& point(int index) const { return points[index]; }

    int contextCount() const { return contexts; }
    const ContextLine& context(int index) const { return contextLines[index]; }
    int islandCount() const { return islands; }
    const MapPoint& island(int index) const { return islandPoints[index]; }

    // Tọa độ trên tuyến theo lý trình, ga mới thêm cũng tự đặt đúng vị trí.
    MapPoint at(double km) const;
    // Lấy đoạn tuyến từ km k0 đến k1, trả về số điểm đã ghi vào out.
    int routeBetween(double k0, double k1, MapPoint* out, int capacity) const;
    const StationMeta* metaOf(int stationID) const;

private:
    MapPoint points[MAX_POINTS];
    int pointCount;
    MapRing rings[MAX_RINGS];
    int ringCount;
    int layerFirst[LAYER_COUNT];
    int layerCount[LAYER_COUNT];
    RoutePoint route[MAX_ROUTE];
    int routeCount;
    StationMeta meta[MAX_META];
    int metaCount;
    ContextLine contextLines[MAX_CONTEXT];
    int contexts;
    MapPoint islandPoints[MAX_ISLANDS];
    int islands;

    void parseLand(MapLayer layer, const std::string& path);
    void parseContext(const std::string& name, char side, float x, float y, const std::string& pts);
};
