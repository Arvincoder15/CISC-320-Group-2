#include "engine/tiled_importer.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <cstdint>
#include <limits>
#include <set>
#include <string>

namespace engine {
namespace {
using Json = nlohmann::json;

void require(bool valid, const std::string& message) {
    if (!valid) {
        throw LevelImportError(message);
    }
}

const Json& required_field(const Json& object, const char* key, const std::string& context) {
    require(object.is_object(), context + " must be an object");
    require(object.contains(key), context + " is missing '" + key + "'");
    return object.at(key);
}

std::string required_string(const Json& object, const char* key, const std::string& context) {
    const auto& value = required_field(object, key, context);
    require(value.is_string(), context + "." + key + " must be a string");
    return value.get<std::string>();
}

std::int64_t positive_integer(const Json& value, const std::string& context) {
    require(value.is_number_integer(), context + " must be a positive integer");
    if (value.is_number_unsigned()) {
        const auto number = value.get<std::uint64_t>();
        require(number <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()),
                context + " is too large");
        return static_cast<std::int64_t>(number);
    }
    const auto number = value.get<std::int64_t>();
    require(number > 0, context + " must be a positive integer");
    return number;
}

double finite_number(const Json& value, const std::string& context) {
    require(value.is_number(), context + " must be a number");
    const auto number = value.get<double>();
    require(std::isfinite(number) &&
                number >= -std::numeric_limits<float>::max() &&
                number <= std::numeric_limits<float>::max(),
            context + " must be finite and representable");
    return number;
}

std::string object_type(const Json& object, const std::string& context) {
    std::string type;
    if (object.contains("class")) {
        require(object.at("class").is_string(), context + ".class must be a string");
        type = object.at("class").get<std::string>();
    }
    if (type.empty() && object.contains("type")) {
        require(object.at("type").is_string(), context + ".type must be a string");
        type = object.at("type").get<std::string>();
    }
    require(!type.empty(), context + " requires a non-empty 'class' or legacy 'type'");
    return type;
}

struct ObjectProperties {
    std::string prefab;
    std::string parent_id;
    std::string linked_id;
};

std::string property_string(const Json& property, const std::string& name, const std::string& context) {
    if (property.contains("type")) {
        require(property.at("type").is_string() &&
                    property.at("type").get<std::string>() == "string",
                context + " property '" + name + "' must have type 'string'");
    }
    const auto& value = required_field(property, "value", context + " property '" + name + "'");
    require(value.is_string(), context + " property '" + name + "' must have a string value");
    return value.get<std::string>();
}

std::string linked_id_property(const Json& property, const std::string& context) {
    const auto& value = required_field(property, "value", context + " property 'linked_id'");
    if (property.contains("type") && property.at("type").is_string() &&
        property.at("type").get<std::string>() == "object") {
        return std::to_string(positive_integer(value, context + " property 'linked_id'"));
    }
    if (property.contains("type")) {
        require(property.at("type").is_string() &&
                    property.at("type").get<std::string>() == "string",
                context + " property 'linked_id' must have type 'string' or 'object'");
    }
    require(value.is_string(), context + " property 'linked_id' must be a string or Tiled object reference");
    return value.get<std::string>();
}

ObjectProperties read_properties(const Json& object, const std::string& context) {
    ObjectProperties result;
    if (!object.contains("properties")) {
        return result;
    }

    const auto& properties = object.at("properties");
    require(properties.is_array(), context + ".properties must be an array");
    std::set<std::string> seen;
    for (const auto& property : properties) {
        const auto name = required_string(property, "name", context + " property");
        require(seen.insert(name).second, context + " has duplicate property '" + name + "'");
        if (name == "prefab") {
            result.prefab = property_string(property, name, context);
        } else if (name == "parent_id") {
            result.parent_id = property_string(property, name, context);
        } else if (name == "linked_id") {
            result.linked_id = linked_id_property(property, context);
        } else {
            throw LevelImportError(context + " has unsupported property '" + name + "'");
        }
    }
    return result;
}

void require_rectangle(const Json& object, const std::string& context) {
    for (const auto* shape : {"ellipse", "point", "polygon", "polyline", "text", "gid", "template"}) {
        require(!object.contains(shape), context + " uses unsupported shape or template field '" + shape + "'");
    }
    if (object.contains("rotation")) {
        require(finite_number(object.at("rotation"), context + ".rotation") == 0.0,
                context + " rotation must be zero");
    }
}

void import_object(const Json& object, LevelDocument& document, std::set<std::string>& tiled_ids) {
    const std::string context = "Tiled object";
    const auto tiled_id = std::to_string(positive_integer(
        required_field(object, "id", context), context + ".id"));
    require(tiled_ids.insert(tiled_id).second, "Duplicate Tiled object id: " + tiled_id);
    require_rectangle(object, context + " " + tiled_id);

    const auto type = object_type(object, context + " " + tiled_id);
    const auto x = finite_number(required_field(object, "x", context + " " + tiled_id), context + " " + tiled_id + ".x");
    const auto y = finite_number(required_field(object, "y", context + " " + tiled_id), context + " " + tiled_id + ".y");
    const auto width = finite_number(required_field(object, "width", context + " " + tiled_id), context + " " + tiled_id + ".width");
    const auto height = finite_number(required_field(object, "height", context + " " + tiled_id), context + " " + tiled_id + ".height");
    require(width > 0.0 && height > 0.0, context + " " + tiled_id + " dimensions must be positive");

    const auto properties = read_properties(object, context + " " + tiled_id);
    if (type == "spawn" || type == "player_spawn") {
        require(!properties.prefab.empty(), context + " " + tiled_id + " spawn requires a 'prefab' property");
        require(properties.parent_id.empty() && properties.linked_id.empty(),
                context + " " + tiled_id + " spawn cannot use object references");
        document.spawns.push_back({properties.prefab, {static_cast<float>(x), static_cast<float>(y)}, tiled_id});
        return;
    }

    document.objects.push_back({
        tiled_id,
        type,
        properties.prefab,
        {static_cast<float>(x), static_cast<float>(y)},
        {static_cast<float>(width), static_cast<float>(height)},
        properties.parent_id,
        properties.linked_id
    });
}

LevelDocument read_map(const Json& map) {
    require(map.is_object(), "Tiled map root must be an object");
    require(required_string(map, "orientation", "Tiled map") == "orthogonal",
            "Tiled map orientation must be 'orthogonal'");
    if (map.contains("infinite")) {
        require(map.at("infinite").is_boolean(), "Tiled map infinite must be a boolean");
        require(!map.at("infinite").get<bool>(), "Infinite Tiled maps are unsupported");
    }

    const auto width = positive_integer(required_field(map, "width", "Tiled map"), "Tiled map.width");
    const auto height = positive_integer(required_field(map, "height", "Tiled map"), "Tiled map.height");
    const auto tile_width = positive_integer(required_field(map, "tilewidth", "Tiled map"), "Tiled map.tilewidth");
    const auto tile_height = positive_integer(required_field(map, "tileheight", "Tiled map"), "Tiled map.tileheight");
    const double pixel_width = static_cast<double>(width) * static_cast<double>(tile_width);
    const double pixel_height = static_cast<double>(height) * static_cast<double>(tile_height);
    require(std::isfinite(pixel_width) && std::isfinite(pixel_height) &&
                pixel_width <= std::numeric_limits<float>::max() &&
                pixel_height <= std::numeric_limits<float>::max(),
            "Tiled map dimensions are too large");

    LevelDocument document;
    document.schema_version = 1;
    document.size = {static_cast<float>(pixel_width), static_cast<float>(pixel_height)};
    if (map.contains("name")) {
        require(map.at("name").is_string(), "Tiled map.name must be a string");
        document.name = map.at("name").get<std::string>();
    }

    const auto& layers = required_field(map, "layers", "Tiled map");
    require(layers.is_array(), "Tiled map.layers must be an array");
    std::set<std::string> tiled_ids;
    for (const auto& layer : layers) {
        const auto layer_type = required_string(layer, "type", "Tiled layer");
        if (layer_type == "tilelayer" || layer_type == "imagelayer") {
            continue;
        }
        require(layer_type == "objectgroup", "Unsupported Tiled layer type: " + layer_type);
        const auto& objects = required_field(layer, "objects", "Tiled object layer");
        require(objects.is_array(), "Tiled object layer.objects must be an array");
        for (const auto& object : objects) {
            import_object(object, document, tiled_ids);
        }
    }

    const auto errors = validate(document);
    if (!errors.empty()) {
        std::string message = "Imported level failed validation";
        for (const auto& error : errors) {
            message += "; " + error;
        }
        throw LevelImportError(message);
    }
    return document;
}
} // namespace

LevelDocument import_tiled_json(std::string_view text, std::string_view source) {
    try {
        return read_map(Json::parse(text.begin(), text.end()));
    } catch (const Json::exception& error) {
        throw LevelImportError("Tiled level '" + std::string(source) + "': invalid JSON or field value: " + error.what());
    } catch (const LevelImportError& error) {
        throw LevelImportError("Tiled level '" + std::string(source) + "': " + error.what());
    }
}
}
