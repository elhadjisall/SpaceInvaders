#pragma once

#include "engine/entity.hpp"

#include <cstdint>
#include <utility>
#include <vector>

// Tightly packed component storage. dense_ has no holes.
// sparse_[entity] is the index into dense_, or kEmpty.
template <typename T>
class Pool {
public:
    static constexpr uint32_t kEmpty = 0xffffffffu;

    void insert(Entity e, const T& value) {
        ensure(e);
        uint32_t idx = sparse_[e];
        if (idx != kEmpty) {
            dense_[idx] = value;
            return;
        }
        sparse_[e] = static_cast<uint32_t>(dense_.size());
        dense_.push_back(value);
        owners_.push_back(e);
    }

    void remove(Entity e) {
        if (e >= sparse_.size()) return;
        uint32_t idx = sparse_[e];
        if (idx == kEmpty) return;
        uint32_t last = static_cast<uint32_t>(dense_.size() - 1);
        if (idx != last) {
            dense_[idx] = std::move(dense_[last]);
            owners_[idx] = owners_[last];
            sparse_[owners_[idx]] = idx;
        }
        dense_.pop_back();
        owners_.pop_back();
        sparse_[e] = kEmpty;
    }

    bool has(Entity e) const {
        return e < sparse_.size() && sparse_[e] != kEmpty;
    }

    T* get(Entity e) {
        if (!has(e)) return nullptr;
        return &dense_[sparse_[e]];
    }

    const T* get(Entity e) const {
        if (!has(e)) return nullptr;
        return &dense_[sparse_[e]];
    }

    size_t size() const { return dense_.size(); }

    Entity entity(size_t i) const { return owners_[i]; }

    T& dense(size_t i) { return dense_[i]; }
    const T& dense(size_t i) const { return dense_[i]; }

    uint32_t index_of(Entity e) const {
        if (!has(e)) return kEmpty;
        return sparse_[e];
    }

    void clear() {
        dense_.clear();
        owners_.clear();
        sparse_.clear();
    }

private:
    void ensure(Entity e) {
        if (sparse_.size() <= e) sparse_.resize(static_cast<size_t>(e) + 1, kEmpty);
    }

    std::vector<T> dense_;
    std::vector<Entity> owners_;
    std::vector<uint32_t> sparse_;
};
