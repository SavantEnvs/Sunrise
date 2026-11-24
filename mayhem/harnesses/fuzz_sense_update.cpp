// svc-8 msg-6 sense update (middleware/bap/activity_message/sense_update.h). decode_sense_update
// binds each wire ClientRef to a native slot through a caller-supplied Resolver (in the server, the
// session's roster). Here the Resolver is driven by the input header so every slot-selected native
// body decoder is reachable:
//   bytes 0..3  sense schema (u32 LE) the slot resolver reports
//   byte  4     resolver outcome: low 2 bits group status, next 2 bits slot status (TargetStatus)
//   rest        the msg-6 body.
// The resolver reads only that immutable header, so each input's result depends on it alone.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <vector>

#include "middleware/bap/activity_message/sense_update.h"

namespace su = sunrise::middleware::bap::activity_message::sense_update;

namespace {
struct Config {
    std::uint32_t schema;
    su::TargetStatus group;
    su::TargetStatus slot;
};
su::TargetStatus status_of(unsigned v) {
    switch (v % 3) {
    case 0: return su::TargetStatus::resolved;
    case 1: return su::TargetStatus::targetUnavailable;
    default: return su::TargetStatus::schemaUnavailable;
    }
}
su::TargetStatus resolve_group(const void* context, std::uint32_t registryKey,
                               su::GroupTarget& output) noexcept {
    const auto* c = static_cast<const Config*>(context);
    output.objectTag = registryKey;
    output.objectRow = registryKey & 0xFFFFU;
    return c->group;
}
su::TargetStatus resolve_slot(const void* context, const su::GroupTarget& group, std::uint8_t,
                              std::uint16_t slotIndex, su::SlotTarget& output) noexcept {
    const auto* c = static_cast<const Config*>(context);
    output.slotRow = group.objectRow + slotIndex;
    output.senseSchema = c->schema;
    output.schemaRow = c->schema;
    return c->slot;
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 5) return -1;
    Config config{};
    std::memcpy(&config.schema, data, 4);
    config.group = status_of(data[4] & 3U);
    config.slot = status_of((data[4] >> 2) & 3U);
    std::vector<std::byte> buf(size - 5);
    if (size > 5) std::memcpy(buf.data(), data + 5, size - 5);
    const su::Resolver resolver{&config, &resolve_group, &resolve_slot};
    auto update = std::make_unique<su::SenseUpdate>();
    std::size_t consumed = 0;
    (void)su::decode_sense_update(std::span<const std::byte>(buf.data(), buf.size()), resolver,
                                  *update, consumed);
    return 0;
}
