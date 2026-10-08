#include "engine/rendering.hpp"
#include "test_support.hpp"
int main() { return run_test([] {
    engine::RecordingRenderer renderer;
    std::vector<engine::SpriteDraw> draws{{1, {2, 3}, "sprite"}};
    renderer.draw(draws);
    draws[0].position.x = 99;
    CHECK(renderer.last_frame()[0].position.x == 2);
    renderer.draw({});
    CHECK(renderer.last_frame().empty());
}); }
