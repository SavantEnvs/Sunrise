// Known-answer selftest of Sunrise's portable codecs (upstream ships no tests). Built by
// mayhem/build.sh with normal flags; run by mayhem/test.sh, which requires one "PASS <name>" line
// per check. Each check asserts decoded VALUES, so a no-op program cannot pass.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "middleware/bap/frame.h"
#include "middleware/web_service/web_service_envelope.h"
#include "state/mods/manifest/mod_manifest_parser.h"
#include "state/mods/manifest/mod_version.h"

namespace bap = sunrise::middleware::bap;
namespace ws = sunrise::middleware::web_service;
namespace manifest = sunrise::state::mods::manifest;
namespace version = sunrise::state::mods::version;

static int failures = 0;
static void check(const char* name, bool ok) {
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) ++failures;
}
static std::vector<std::byte> bytes(std::initializer_list<int> v) {
    std::vector<std::byte> out;
    for (int b : v) out.push_back(static_cast<std::byte>(b));
    return out;
}

int main() {
    {   // Web Service header: big-endian opcode 1901, txid 0x0102032A, 2 payload bytes.
        auto in = bytes({0x07, 0x6D, 0x01, 0x02, 0x03, 0x2A, 0xAA, 0xBB});
        ws::Message m{};
        bool ok = ws::parse_request(in, m);
        check("web_service_header", ok && m.opcode == 1901 && m.transactionId == 0x0102032Au &&
                                        m.payload.size() == 2 && m.payload[0] == std::byte{0xAA});
        auto shortIn = bytes({0x07, 0x6D, 0x01});
        check("web_service_header_short_rejected", !ws::parse_request(shortIn, m));
    }
    {   // BAP request payload round trip: encode, then parse back service, task and body.
        auto body = bytes({1, 2, 3, 4, 5});
        std::vector<std::byte> out(64);
        std::size_t written = 0;
        bool enc = bap::encode_request_payload(bap::RequestService::webService, 77, body, out, written);
        bap::RequestFrame rf{};
        bool dec = enc && bap::parse_request_payload(std::span<const std::byte>(out.data(), written),
                                                     bap::FrameType::plaintext0, rf);
        check("bap_request_roundtrip", dec && rf.serviceId == 10 && rf.taskId == 77 &&
                                           rf.body.size() == 5 && rf.body[4] == std::byte{5});
    }
    {   // Mod version grammar.
        sunrise::state::mods::Version v{};
        bool ok = version::parse("12.0.345", v);
        check("mod_version_parse", ok && v.major == 12 && v.minor == 0 && v.patch == 345);
        check("mod_version_leading_zero_rejected", !version::parse("01.2.3", v));
        sunrise::state::mods::Version w{};
        check("mod_version_compare", version::parse("12.1.0", w) && version::compare(v, w) < 0 &&
                                         version::compare(w, v) > 0 && version::compare(v, v) == 0);
        auto t = version::text(w);
        check("mod_version_text", std::string_view(t.data()) == "12.1.0");
    }
    {   // Mod manifest: version probe and a full minimal manifest.
        constexpr std::string_view json =
            R"({"manifestVersion": 4, "id": "example.mod", "version": "1.2.3", "name": "Example"})";
        std::uint32_t mv = 0;
        check("manifest_read_version", manifest::read_version(json, mv) && mv == 4);
        auto parsed = std::make_unique<sunrise::state::mods::Manifest>();
        auto error = std::make_unique<manifest::Error>();
        bool ok = manifest::parse(json, *parsed, *error);
        check("manifest_parse", ok && parsed->manifestVersion == 4 && parsed->version.major == 1 &&
                                    parsed->version.minor == 2 && parsed->version.patch == 3);
        check("manifest_unknown_key_rejected",
              !manifest::parse(R"({"manifestVersion": 4, "id": "x.y", "bogus": 1})", *parsed, *error));
    }
    return failures == 0 ? 0 : 1;
}
