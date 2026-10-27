#include <cstdio>

int test_ecs();
int test_spatial();
int test_batch();

int main() {
    int failed = 0;
    failed |= test_ecs();
    failed |= test_spatial();
    failed |= test_batch();
    if (failed) {
        std::fprintf(stderr, "failed\n");
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
