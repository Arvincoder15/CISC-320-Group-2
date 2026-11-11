#include "engine/tools.hpp"
#include "engine/tiled_importer.hpp"
#include "test_support.hpp"
#include <fstream>
#include <iterator>
#include <string>

namespace {
std::string read_fixture(const char* name) {
    const std::string path = std::string("fixtures/") + name;
    std::ifstream input(path, std::ios::binary);
    CHECK(input.good());
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

bool import_error_contains(const std::string& json, const std::string& expected) {
    try {
        (void)engine::import_tiled_json(json, "test-map.json");
    } catch (const engine::LevelImportError& error) {
        return std::string(error.what()).find(expected) != std::string::npos;
    }
    return false;
}
}

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

    const auto imported = engine::import_tiled_json(read_fixture("tiled_level.json"), "fixtures/tiled_level.json");
    CHECK(imported.schema_version == 1);
    CHECK(imported.name == "Cooperative Puzzle Room");
    CHECK(near(imported.size.x, 1280.0F));
    CHECK(near(imported.size.y, 768.0F));
    CHECK(imported.spawns.size() == 2);
    CHECK(imported.spawns[0].id == "101");
    CHECK(imported.spawns[0].prefab == "fire_player");
    CHECK(near(imported.spawns[1].position.x, 96.0F));
    CHECK(imported.objects.size() == 6);
    CHECK(imported.objects[0].type == "platform");
    CHECK(imported.objects[2].type == "pressure_plate");
    CHECK(imported.objects[2].linked_id == "204");
    CHECK(imported.objects[4].prefab == "fire_exit");
    CHECK(engine::validate(imported).empty());

    CHECK(import_error_contains("{", "invalid JSON"));
    CHECK(import_error_contains("{", "test-map.json"));
    CHECK(import_error_contains(R"({"orientation":"isometric"})", "orthogonal"));
    CHECK(import_error_contains(
        R"({"orientation":"orthogonal","width":1,"height":1,"tilewidth":16,"tileheight":16,"layers":[{"type":"objectgroup","objects":[{"id":1,"class":"player_spawn","x":0,"y":0,"width":1,"height":1}]}]})",
        "requires a 'prefab' property"));
    CHECK(import_error_contains(
        R"({"orientation":"orthogonal","width":1,"height":1,"tilewidth":16,"tileheight":16,"layers":[{"type":"objectgroup","objects":[{"id":1,"class":"pressure_plate","x":0,"y":0,"width":1,"height":1,"properties":[{"name":"linked_id","type":"object","value":99}]}]}]})",
        "missing linked id: 99"));
    CHECK(import_error_contains(
        R"({"orientation":"orthogonal","width":1,"height":1,"tilewidth":16,"tileheight":16,"layers":[{"type":"objectgroup","objects":[{"id":1,"class":"platform","x":0,"y":0,"width":0,"height":1}]}]})",
        "dimensions must be positive"));
    CHECK(import_error_contains(
        R"({"orientation":"orthogonal","width":1,"height":1,"tilewidth":16,"tileheight":16,"layers":[{"type":"objectgroup","objects":[{"id":1,"class":"platform","x":0,"y":0,"width":1,"height":1,"properties":[{"name":"unknown","type":"string","value":"ignored"}]}]}]})",
        "unsupported property 'unknown'"));
    CHECK(import_error_contains(
        R"({"orientation":"orthogonal","width":1,"height":1,"tilewidth":16,"tileheight":16,"layers":[{"type":"group","layers":[]}]})",
        "Unsupported Tiled layer type: group"));
    CHECK(import_error_contains(
        R"({"orientation":"orthogonal","width":1,"height":1,"tilewidth":16,"tileheight":16,"layers":[{"type":"objectgroup","objects":[{"id":1,"class":"platform","x":0,"y":0,"width":1,"height":1},{"id":1,"type":"exit","x":0,"y":0,"width":1,"height":1}]}]})",
        "Duplicate Tiled object id: 1"));
    CHECK(import_error_contains(
        R"({"orientation":"orthogonal","width":1,"height":1,"tilewidth":16,"tileheight":16,"layers":[{"type":"objectgroup","objects":[{"id":1,"class":"platform","x":0,"y":0,"width":1,"height":1,"point":true}]}]})",
        "unsupported shape or template field 'point'"));
    CHECK(import_error_contains(
        R"({"orientation":"orthogonal","width":1,"height":1,"tilewidth":16,"tileheight":16,"layers":[{"type":"objectgroup","objects":[{"id":1,"class":"player_spawn","x":0,"y":0,"width":1,"height":1,"properties":[{"name":"prefab","type":"string","value":"player"},{"name":"linked_id","type":"string","value":"3"}]}]}]})",
        "spawn cannot use object references"));
}); }
