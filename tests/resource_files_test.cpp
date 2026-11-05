#include "engine/resource_files.hpp"
#include "test_support.hpp"
#include "filesystem_test_support.hpp"

int main() { return run_test([] {
    TestDirectory temporary;
    const auto root = temporary.path / "assets";
    fs::create_directories(root / "textures");
    std::vector<char> binary(17000, 'x'); // Multiple read chunks and embedded zero.
    binary[0] = '\0'; binary[8192] = '\0'; binary.back() = 'z';
    write(root / "textures/player.dat", binary);
    write(root / "empty.dat", {});
    engine::FileResourceManager manager(root);
    auto asset = manager.load("textures/player.dat");
    CHECK(asset->bytes == binary);
    CHECK(asset->source == fs::canonical(root / "textures/player.dat"));
    CHECK(manager.load("textures/./player.dat") == asset);
    CHECK(manager.load("textures/../textures/player.dat") == asset);
    CHECK(manager.size() == 1);
    CHECK(manager.load("empty.dat")->bytes.empty());
    CHECK(throws_as<engine::ResourceError>([&] { manager.load(""); }));
    CHECK(throws_as<engine::ResourceError>([&] { manager.load(root / "empty.dat"); }));
    CHECK(throws_as<engine::ResourceError>([&] { manager.load("textures"); }));
    CHECK(throws_as<engine::ResourceError>([&] { manager.load("../outside.dat"); }));
    CHECK(throws_as<engine::ResourceError>([&] { manager.load(std::string("empty.dat\0extra", 15)); }));
    bool reported = false;
    try { manager.load("missing.dat"); }
    catch (const engine::ResourceError& error) {
        const std::string message(error.what());
        reported = message.find("missing.dat") != std::string::npos && message.find("Resource") != std::string::npos;
    }
    CHECK(reported && manager.size() == 2);
    write(root / "missing.dat", {'o', 'k'});
    CHECK(manager.load("missing.dat")->bytes == std::vector<char>({'o', 'k'}));
    // Cached content is a snapshot until clear(), even if the source changes.
    write(root / "textures/player.dat", {'n', 'e', 'w'});
    CHECK(manager.load("textures/player.dat") == asset);
    std::weak_ptr<const engine::FileResource> weak = asset;
    manager.clear();
    CHECK(manager.size() == 0 && !weak.expired());
    CHECK(asset->bytes == binary);
    const auto refreshed = manager.load("textures/player.dat");
    CHECK(refreshed != asset && refreshed->bytes == std::vector<char>({'n', 'e', 'w'}));
    asset.reset();
    CHECK(weak.expired());
    engine::FileResourceManager::Handle surviving;
    {
        engine::FileResourceManager shorter_lived(root);
        surviving = shorter_lived.load("empty.dat");
    }
    CHECK(surviving && surviving->bytes.empty());
    CHECK(throws_as<engine::ResourceError>([&] { engine::FileResourceManager bad(root / "absent"); }));
    CHECK(throws_as<engine::ResourceError>([&] { engine::FileResourceManager bad(root / "empty.dat"); }));
    // Symlink creation can require privileges on Windows; report that case explicitly.
    std::error_code error;
    fs::create_symlink(root / "textures/player.dat", root / "alias.dat", error);
    if (!error) {
        CHECK(manager.load("alias.dat") == refreshed);
        write(temporary.path / "outside.dat", {'x'});
        fs::create_symlink(temporary.path / "outside.dat", root / "outside-link.dat");
        CHECK(throws_as<engine::ResourceError>([&] { manager.load("outside-link.dat"); }));
    } else {
        std::cout << "SKIP symlink checks: " << error.message() << '\n';
    }
}); }
