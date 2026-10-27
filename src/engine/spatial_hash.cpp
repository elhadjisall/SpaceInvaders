#include "engine/spatial_hash.hpp"

#include <cmath>
#include <cstdint>
#include <unordered_set>

SpatialHash::SpatialHash(float cell_size) : cell_size_(cell_size) {}

void SpatialHash::clear() {
    cells_.clear();
    stats_ = HashStats{};
}

int SpatialHash::cell_of(float v) const {
    return static_cast<int>(std::floor(v / cell_size_));
}

int SpatialHash::cell_of_max(float max_v) const {
    return static_cast<int>(std::floor((max_v - 0.001f) / cell_size_));
}

void SpatialHash::insert(Entity e, float x, float y, float w, float h) {
    if (w < 0.f) w = 0.f;
    if (h < 0.f) h = 0.f;
    int x0 = cell_of(x);
    int y0 = cell_of(y);
    int x1 = cell_of_max(x + w);
    int y1 = cell_of_max(y + h);
    if (x1 < x0) x1 = x0;
    if (y1 < y0) y1 = y0;

    Item item;
    item.entity = e;
    item.x = x;
    item.y = y;
    item.w = w;
    item.h = h;

    for (int cy = y0; cy <= y1; ++cy) {
        for (int cx = x0; cx <= x1; ++cx) {
            cells_[CellKey{cx, cy}].push_back(item);
        }
    }
}

bool SpatialHash::overlap(const Item& a, const Item& b) {
    return a.x < b.x + b.w && a.x + a.w > b.x &&
           a.y < b.y + b.h && a.y + a.h > b.y;
}

void SpatialHash::query(std::vector<HashPair>& out) {
    out.clear();
    stats_ = HashStats{};
    stats_.cells = static_cast<int>(cells_.size());

    // Forward neighbors cover each adjacent cell pair once.
    static const int kOff[4][2] = {{1, 0}, {1, 1}, {0, 1}, {-1, 1}};

    std::unordered_set<uint64_t> seen;
    seen.reserve(cells_.size() * 8 + 16);

    auto consider = [&](const Item& a, const Item& b) {
        if (a.entity == b.entity) return;
        uint32_t lo = a.entity < b.entity ? a.entity : b.entity;
        uint32_t hi = a.entity < b.entity ? b.entity : a.entity;
        uint64_t key = (static_cast<uint64_t>(lo) << 32) | hi;
        if (!seen.insert(key).second) return;
        ++stats_.pair_tests;
        if (!overlap(a, b)) return;
        out.push_back(HashPair{lo, hi});
        ++stats_.pairs;
    };

    for (const auto& entry : cells_) {
        const std::vector<Item>& items = entry.second;
        for (size_t i = 0; i < items.size(); ++i) {
            for (size_t j = i + 1; j < items.size(); ++j) {
                consider(items[i], items[j]);
            }
        }
        for (const auto& off : kOff) {
            CellKey neighbor{entry.first.x + off[0], entry.first.y + off[1]};
            auto it = cells_.find(neighbor);
            if (it == cells_.end()) continue;
            for (const Item& a : items) {
                for (const Item& b : it->second) {
                    consider(a, b);
                }
            }
        }
    }
}

void SpatialHash::debug_cells(std::vector<CellDebug>& out) const {
    out.clear();
    out.reserve(cells_.size());
    for (const auto& entry : cells_) {
        CellDebug cell;
        cell.x = entry.first.x;
        cell.y = entry.first.y;
        cell.count = static_cast<int>(entry.second.size());
        cell.world_x = static_cast<float>(entry.first.x) * cell_size_;
        cell.world_y = static_cast<float>(entry.first.y) * cell_size_;
        cell.world_w = cell_size_;
        cell.world_h = cell_size_;
        out.push_back(cell);
    }
}
