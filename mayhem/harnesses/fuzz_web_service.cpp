// Web Service (svc-10) request decoding: the 6-byte opcode/txid envelope, then the bit-packed
// request codec the server dispatches on that opcode (server/web_service/web_service_actions.cpp).
// An opcode with no codec here is decoded by every codec in turn, so each is reachable by any input.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include "middleware/web_service/web_service_envelope.h"
#include "middleware/web_service/messages/opcode104.h"
#include "middleware/web_service/messages/opcode1801.h"
#include "middleware/web_service/messages/opcode1820.h"
#include "middleware/web_service/messages/opcode1821.h"
#include "middleware/web_service/messages/opcode1901.h"
#include "middleware/web_service/messages/opcode2002.h"
#include "middleware/web_service/messages/opcode205.h"
#include "middleware/web_service/messages/opcode206.h"
#include "middleware/web_service/messages/opcode2400.h"
#include "middleware/web_service/messages/opcode402.h"
#include "middleware/web_service/messages/opcode403.h"
#include "middleware/web_service/messages/opcode405.h"
#include "middleware/web_service/messages/opcode406.h"
#include "middleware/web_service/messages/opcode501_request_codec.h"
#include "middleware/web_service/messages/opcode502.h"
#include "middleware/web_service/messages/opcode503.h"
#include "middleware/web_service/messages/opcode504.h"
#include "middleware/web_service/messages/opcode505/opcode505_codec.h"
#include "middleware/web_service/messages/opcode601/opcode601_codec.h"
#include "middleware/web_service/messages/opcode701/opcode701_codec.h"
#include "middleware/web_service/messages/opcode702.h"
#include "middleware/web_service/messages/opcode801.h"
#include "middleware/web_service/messages/opcode901/opcode901_codec.h"
#include "middleware/web_service/messages/opcode903.h"
#include "middleware/web_service/messages/opcode904/opcode904_codec.h"

namespace ws = sunrise::middleware::web_service;
namespace msg = sunrise::middleware::web_service::messages;

namespace {
template <class T> bool run(bool (*f)(const ws::Message&, T&) noexcept, const ws::Message& m) {
    auto out = std::make_unique<T>();
    return f(m, *out);
}
bool run(bool (*f)(const ws::Message&) noexcept, const ws::Message& m) { return f(m); }

struct Codec {
    std::uint16_t opcode;
    bool (*decode)(const ws::Message&);
};
const Codec kCodecs[] = {
    {104, [](const ws::Message& m) { return run(&msg::opcode104::parse_request, m); }},
    {1801, [](const ws::Message& m) { return run(&msg::opcode1801::parse_request, m); }},
    {1820, [](const ws::Message& m) { return run(&msg::opcode1820::parse_request, m); }},
    {1821, [](const ws::Message& m) { return run(&msg::opcode1821::parse_request, m); }},
    {1901, [](const ws::Message& m) { return run(&msg::opcode1901::parse_request, m); }},
    {2002, [](const ws::Message& m) { return run(&msg::opcode2002::parse_request, m); }},
    {205, [](const ws::Message& m) { return run(&msg::opcode205::parse_request, m); }},
    {206, [](const ws::Message& m) { return run(&msg::opcode206::parse_request, m); }},
    {2400, [](const ws::Message& m) { return run(&msg::opcode2400::parse_request, m); }},
    {402, [](const ws::Message& m) { return run(&msg::opcode402::parse_request, m); }},
    {403, [](const ws::Message& m) { return run(&msg::opcode403::parse_request, m); }},
    {405, [](const ws::Message& m) { return run(&msg::opcode405::parse_request, m); }},
    {406, [](const ws::Message& m) { return run(&msg::opcode406::parse_request, m); }},
    {501, [](const ws::Message& m) { return run(&msg::opcode501::parse_request, m); }},
    {502, [](const ws::Message& m) { return run(&msg::opcode502::parse_request, m); }},
    {503, [](const ws::Message& m) { return run(&msg::opcode503::parse_request, m); }},
    {504, [](const ws::Message& m) { return run(&msg::opcode504::parse_request, m); }},
    {505, [](const ws::Message& m) { return run(&msg::opcode505::parse_request, m); }},
    {601, [](const ws::Message& m) { return run(&msg::opcode601::parse_request, m); }},
    {701, [](const ws::Message& m) { return run(&msg::opcode701::parse_request, m); }},
    {702, [](const ws::Message& m) { return run(&msg::opcode702::parse_request, m); }},
    {801, [](const ws::Message& m) { return run(&msg::opcode801::parse_request, m); }},
    {901, [](const ws::Message& m) { return run(&msg::opcode901::parse_request, m); }},
    {903, [](const ws::Message& m) { return run(&msg::opcode903::parse_request, m); }},
    {904, [](const ws::Message& m) { return run(&msg::opcode904::parse_request, m); }},
};
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Exactly-sized heap copy so ASan sees reads past the body the transport hands over.
    std::vector<std::byte> body(size);
    if (size) std::memcpy(body.data(), data, size);
    ws::Message message{};
    if (!ws::parse_request(std::span<const std::byte>(body.data(), body.size()), message)) return 0;
    bool matched = false;
    for (const Codec& c : kCodecs) {
        if (c.opcode == message.opcode) {
            (void)c.decode(message);
            matched = true;
        }
    }
    if (!matched)
        for (const Codec& c : kCodecs) (void)c.decode(message);
    return 0;
}
