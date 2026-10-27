#pragma once

#include "engine/entity.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

struct HashPair {
    Entity a;
    Entity b;
};

struct HashStats {
    int cells = 0;
    int pair_tests = 0;
    int pairs = 0;
};

struct CellDebug {
    int x = 0;
    int y = 0;
    int count = 0;
    float world_x = 0.f;
    float world_y = 0.f;
    float world_w = 0.f;
    float world_h = 0.f;
};

// Uniform grid. An AABB is inserted into every cell it touches.
// query() runs the narrowphase only for entities that share a cell or sit in
// neighboring cells.
class SpatialHash {
public:
    explicit SpatialHash(float cell_size);

    void clear();
    void insert(Entity e, float x, float y, float w, float h);
    void query(std::vector<HashPair>& out);

    const HashStats& stats() const { return stats_; }
    int cell_count() const { return static_cast<int>(cells_.size()); }
    float cell_size() const { return cell_size_; }
    void debug_cells(std::vector<CellDebug>& out) const;

private:
    struct Item {
        Entity entity;
        float x, y, w, h;
    };

    struct CellKey {
        int x;
        int y;
        bool operator==(const CellKey& o) const { return x == o.x && y == o.y; }
    };

    struct CellKeyHash {
        size_t operator()(const CellKey& k) const noexcept {
            uint32_t x = static_cast<uint32_t>(k.x);
            uint32_t y = static_cast<uint32_t>(k.y);
            return (static_cast<size_t>(x) * 0x9E3779B1u) ^ (static_cast<size_t>(y) * 0x85EBCA77u);
        }
    };

    int cell_of(float v) const;
    int cell_of_max(float max_v) const;
    static bool overlap(const Item& a, const Item& b);

    float cell_size_;
    std::unordered_map<CellKey, std::vector<Item>, CellKeyHash> cells_;
    HashStats stats_;
};
