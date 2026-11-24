// Third-party mod manifest (mod.json) parsing: the version probe, the full manifest parser with its
// bindings, and the dependency version grammar -- what state/mods discovery runs on every mod file.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>

#include "state/mods/manifest/mod_manifest_bindings.h"
#include "state/mods/manifest/mod_manifest_parser.h"
#include "state/mods/manifest/mod_version.h"

namespace manifest = sunrise::state::mods::manifest;
namespace version = sunrise::state::mods::version;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Exactly-sized heap copy (no terminator): the parsers take a string_view.
    std::unique_ptr<char[]> text(new char[size ? size : 1]);
    if (size) std::memcpy(text.get(), data, size);
    const std::string_view json(text.get(), size);

    std::uint32_t manifestVersion = 0;
    (void)manifest::read_version(json, manifestVersion);
    {
        auto parsed = std::make_unique<sunrise::state::mods::Manifest>();
        auto error = std::make_unique<manifest::Error>();
        (void)manifest::parse(json, *parsed, *error);
    }
    {
        auto bound = std::make_unique<sunrise::state::mods::Manifest>();
        auto error = std::make_unique<manifest::Error>();
        (void)manifest::read_bindings(json, *bound, *error);
    }
    sunrise::state::mods::Version v{};
    if (version::parse(json, v)) (void)version::text(v);
    return 0;
}
