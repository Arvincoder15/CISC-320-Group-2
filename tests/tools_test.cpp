#include "engine/tools.hpp"
#include "engine/tiled_importer.hpp"
#include "test_support.hpp"
#include "filesystem_test_support.hpp"
#include <fstream>
#include <iterator>
#include <limits>
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

bool document_error_contains(const std::string& json, const std::string& source, const std::string& expected) {
    try {
        (void)engine::parse_level_json(json, source);
    } catch (const engine::LevelDocumentError& error) {
        const std::string message(error.what());
        return message.find(source) != std::string::npos && message.find(expected) != std::string::npos;
    }
    return false;
}

engine::LevelDocument persistence_sample() {
    engine::LevelDocument document;
    document.name = "Persistence room";
    document.size = {960.0F, 540.0F};
    document.spawns = {
        {"fire_player", {64.0F, 500.0F}, "spawn-fire"},
        {"water_player", {128.0F, 500.0F}, ""}
    };
    document.objects = {
        {"platform-1", "platform", "", {0.0F, 520.0F}, {960.0F, 20.0F}, "", ""},
        {"door-1", "door", "door-prefab", {700.0F, 420.0F}, {32.0F, 100.0F}, "", ""},
        {"plate-1", "pressure_plate", "plate-prefab", {180.0F, 510.0F}, {32.0F, 10.0F}, "platform-1", "door-1"}
    };
    return document;
}

void check_same_level(const engine::LevelDocument& actual, const engine::LevelDocument& expected) {
    CHECK(actual.schema_version == expected.schema_version);
    CHECK(actual.name == expected.name);
    CHECK(near(actual.size.x, expected.size.x));
    CHECK(near(actual.size.y, expected.size.y));
    CHECK(actual.spawns.size() == expected.spawns.size());
    for (std::size_t i = 0; i < expected.spawns.size(); ++i) {
        CHECK(actual.spawns[i].id == expected.spawns[i].id);
        CHECK(actual.spawns[i].prefab == expected.spawns[i].prefab);
        CHECK(near(actual.spawns[i].position.x, expected.spawns[i].position.x));
        CHECK(near(actual.spawns[i].position.y, expected.spawns[i].position.y));
    }
    CHECK(actual.objects.size() == expected.objects.size());
    for (std::size_t i = 0; i < expected.objects.size(); ++i) {
        CHECK(actual.objects[i].id == expected.objects[i].id);
        CHECK(actual.objects[i].type == expected.objects[i].type);
        CHECK(actual.objects[i].prefab == expected.objects[i].prefab);
        CHECK(near(actual.objects[i].position.x, expected.objects[i].position.x));
        CHECK(near(actual.objects[i].position.y, expected.objects[i].position.y));
        CHECK(near(actual.objects[i].size.x, expected.objects[i].size.x));
        CHECK(near(actual.objects[i].size.y, expected.objects[i].size.y));
        CHECK(actual.objects[i].parent_id == expected.objects[i].parent_id);
        CHECK(actual.objects[i].linked_id == expected.objects[i].linked_id);
    }
}

std::string read_text_file(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    CHECK(input.good());
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void write_text_file(const fs::path& path, const std::string& text) {
    write(path, std::vector<char>(text.begin(), text.end()));
}

std::string replace_once(std::string text, const std::string& from, const std::string& to) {
    const auto position = text.find(from);
    CHECK(position != std::string::npos);
    text.replace(position, from.size(), to);
    return text;
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

    auto persistence_document = persistence_sample();
    const auto serialized = engine::serialize_level_json(persistence_document);
    CHECK(serialized.find("\"format\": \"cisc320.level-document\"") != std::string::npos);
    check_same_level(engine::parse_level_json(serialized, "round-trip.level.json"), persistence_document);

    CHECK(document_error_contains("{", "broken.level.json", "invalid JSON"));
    CHECK(document_error_contains(
        R"({"format":"cisc320.level-document","schema_version":2})",
        "future.level.json", "schema_version must be integer 1"));
    CHECK(document_error_contains(
        replace_once(serialized, "\"format\": \"cisc320.level-document\"", "\"format\": \"tiled\""),
        "wrong-format.json", "format must be 'cisc320.level-document'"));
    auto unknown_root_field = serialized;
    unknown_root_field.insert(unknown_root_field.rfind('}'), ",\n  \"unexpected\": true\n");
    CHECK(document_error_contains(unknown_root_field, "unknown-field.level.json", "unsupported field 'unexpected'"));
    CHECK(document_error_contains(
        replace_once(serialized, "\"type\": \"platform\"", "\"type\": \"platform\",\n      \"unexpected\": true"),
        "unknown-object-field.level.json", "unsupported field 'unexpected'"));
    CHECK(document_error_contains(
        replace_once(serialized, "\"linked_id\": \"door-1\"", "\"linked_id\": \"missing-door\""),
        "broken-link.level.json", "missing linked id: missing-door"));
    CHECK(document_error_contains(
        replace_once(serialized, "\"size\": {\n        \"x\": 960.0,\n        \"y\": 20.0",
                     "\"size\": {\n        \"x\": 0.0,\n        \"y\": 20.0"),
        "invalid-size.level.json", "size must be a positive finite rectangle"));
    CHECK(document_error_contains(
        replace_once(serialized, "\"name\": \"Persistence room\"", "\"name\": 42"),
        "wrong-type.level.json", "name must be a string"));

    auto invalid_bounds = persistence_document;
    invalid_bounds.size.x = std::numeric_limits<float>::infinity();
    CHECK(!engine::validate(invalid_bounds).empty());
    CHECK(throws_as<engine::LevelDocumentError>([&] { engine::serialize_level_json(invalid_bounds); }));

    TestDirectory level_directory;
    const auto level_path = level_directory.path / "room.level.json";
    engine::save_level_file(level_path, persistence_document);
    check_same_level(engine::load_level_file(level_path), persistence_document);

    auto edited_document = persistence_document;
    edited_document.name = "Edited room";
    edited_document.objects[2].linked_id = "door-1";
    engine::save_level_file(level_path, edited_document);
    auto active_document = persistence_document;
    engine::reload_level(active_document, level_path);
    check_same_level(active_document, edited_document);

    const auto active_before_failure = active_document;
    write_text_file(level_path, "{invalid");
    CHECK(throws_as<engine::LevelDocumentError>([&] { engine::reload_level(active_document, level_path); }));
    check_same_level(active_document, active_before_failure);
    write_text_file(level_path, serialize_level_json(persistence_document));
    auto invalid_link_json = replace_once(
        read_text_file(level_path), "\"linked_id\": \"door-1\"", "\"linked_id\": \"missing-door\"");
    write_text_file(level_path, invalid_link_json);
    CHECK(throws_as<engine::LevelDocumentError>([&] { engine::reload_level(active_document, level_path); }));
    check_same_level(active_document, active_before_failure);
    CHECK(throws_as<engine::LevelFileError>([&] {
        engine::reload_level(active_document, level_directory.path / "missing.level.json");
    }));
    check_same_level(active_document, active_before_failure);

    write_text_file(level_path, engine::serialize_level_json(edited_document));
    const auto saved_bytes = read_text_file(level_path);
    auto invalid_to_save = edited_document;
    invalid_to_save.size.y = std::numeric_limits<float>::quiet_NaN();
    CHECK(throws_as<engine::LevelDocumentError>([&] { engine::save_level_file(level_path, invalid_to_save); }));
    CHECK(read_text_file(level_path) == saved_bytes);
    CHECK(throws_as<engine::LevelFileError>([&] {
        engine::save_level_file(level_directory.path / "missing" / "room.level.json", edited_document);
    }));
    CHECK(read_text_file(level_path) == saved_bytes);
    CHECK(throws_as<engine::LevelFileError>([&] { engine::load_level_file(level_directory.path); }));
    CHECK(throws_as<engine::LevelFileError>([&] { engine::load_level_file({}); }));
    CHECK(throws_as<engine::LevelFileError>([&] { engine::save_level_file({}, edited_document); }));
    CHECK(throws_as<engine::LevelFileError>([&] { engine::save_level_file(level_directory.path, edited_document); }));

    const auto linked_path = level_directory.path / "linked.level.json";
    std::error_code symlink_error;
    fs::create_symlink(level_path, linked_path, symlink_error);
    if (!symlink_error) {
        CHECK(throws_as<engine::LevelFileError>([&] { engine::load_level_file(linked_path); }));
        CHECK(throws_as<engine::LevelFileError>([&] { engine::save_level_file(linked_path, edited_document); }));
        CHECK(read_text_file(level_path) == saved_bytes);
    } else {
        std::cout << "SKIP: level file symlink checks: " << symlink_error.message() << '\n';
    }

    for (const auto& entry : fs::directory_iterator(level_directory.path)) {
        CHECK(entry.path().filename().string().find(".level-save-") != 0);
    }
}); }
