#include "engine/world.hpp"

#include <cstdio>
#include <vector>

int test_ecs() {
    World world;
    std::vector<Entity> ids;
    ids.reserve(300);
    for (int i = 0; i < 300; ++i) {
        Entity e = world.create();
        if (e != static_cast<Entity>(i)) {
            std::fprintf(stderr, "ecs: entity id %u is not the slot index\n", e);
            return 1;
        }
        world.transforms.insert(e, Transform{static_cast<float>(i), 0.f});
        world.velocities.insert(e, Velocity{1.f, 0.f});
        ids.push_back(e);
    }
    if (world.transforms.size() != 300 || world.alive_count() != 300) {
        std::fprintf(stderr, "ecs: expected 300 packed transforms\n");
        return 1;
    }

    for (int i = 0; i < 300; i += 2) world.destroy(ids[static_cast<size_t>(i)]);

    if (world.alive_count() != 150 || world.transforms.size() != 150 || world.velocities.size() != 150) {
        std::fprintf(stderr, "ecs: destroy left a hole, alive=%zu transforms=%zu\n",
                     world.alive_count(), world.transforms.size());
        return 1;
    }

    float sum = 0.f;
    for (size_t i = 0; i < world.velocities.size(); ++i) {
        Entity e = world.velocities.entity(i);
        if (!world.alive(e)) {
            std::fprintf(stderr, "ecs: packed velocity points at a dead entity\n");
            return 1;
        }
        if (world.velocities.index_of(e) != static_cast<uint32_t>(i)) {
            std::fprintf(stderr, "ecs: sparse index does not match the packed slot\n");
            return 1;
        }
        Transform* t = world.transforms.get(e);
        if (!t) {
            std::fprintf(stderr, "ecs: living entity lost its transform\n");
            return 1;
        }
        t->x += world.velocities.dense(i).x;
        sum += t->x;
    }

    Entity recycled = world.create();
    if (world.transforms.get(recycled) != nullptr) {
        std::fprintf(stderr, "ecs: recycled entity kept a component\n");
        return 1;
    }
    if (world.generation(recycled) == 1) {
        std::fprintf(stderr, "ecs: recycled entity did not bump generation\n");
        return 1;
    }

    std::printf("ok ecs alive=%zu packed=%zu sum=%.0f\n", world.alive_count(), world.transforms.size(), static_cast<double>(sum));
    return 0;
}
