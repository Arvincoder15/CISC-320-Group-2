#include "engine/tools.hpp"
#include "test_support.hpp"
int main() { return run_test([] {
    CHECK(engine::validate(engine::LevelDocument{1, {{"player", {0.0F, 0.0F}, "spawn-1"}}, "", {0.0F, 0.0F}, {}}).empty());
    CHECK(engine::validate(engine::LevelDocument{2, {}, "", {0.0F, 0.0F}, {}}).size() == 1);
    CHECK(engine::validate(engine::LevelDocument{1, {{"", {0.0F, 0.0F}, "spawn-1"}}, "", {0.0F, 0.0F}, {}}).size() == 1);

    engine::LevelDocument valid_document{};
    valid_document.schema_version = 1;
    valid_document.spawns.push_back({"player", {0.0F, 0.0F}, "spawn-1"});
    valid_document.objects.push_back({"door-1", "door", "door_prefab", {10.0F, 5.0F}, {2.0F, 1.0F}, "", ""});
    valid_document.objects.push_back({"plate-1", "pressure_plate", "plate_prefab", {10.0F, 12.0F}, {1.0F, 1.0F}, "", "door-1"});
    CHECK(engine::validate(valid_document).empty());

    engine::LevelDocument invalid_link_document = valid_document;
    invalid_link_document.objects.push_back({"broken-link", "pressure_plate", "plate_prefab", {5.0F, 5.0F}, {1.0F, 1.0F}, "", "missing-door"});
    CHECK(engine::validate(invalid_link_document).size() > 0);

    engine::LevelDocument bad_size_document = valid_document;
    bad_size_document.objects[0].size = {0.0F, 2.0F};
    CHECK(engine::validate(bad_size_document).size() > 0);
}); }
