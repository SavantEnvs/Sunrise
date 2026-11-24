// Item-definition blob readers (middleware/content/packages/tables/items.h), split out of
// content-tables so the other package-table decoders are not drowned by findings here.
// Input byte 0 picks the entry point; the rest is the definition (and, for the plug-set
// visitors, the shared plug-set table) the reader is given.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <vector>

#include "middleware/content/packages/tables/items.h"

namespace items = sunrise::middleware::content::packages::tables::items;
using Bytes = std::span<const std::byte>;

namespace {
bool count_plug(void* context, std::uint32_t) noexcept {
    ++*static_cast<std::size_t*>(context);
    return true;
}
void count_flag(void* context, std::uint32_t, std::int16_t) noexcept {
    ++*static_cast<std::size_t*>(context);
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1) return -1;
    const uint8_t op = data[0] % 5;
    // Exactly-sized heap copy so ASan sees any read past the declared end.
    std::vector<std::byte> buf(size - 1);
    if (size > 1) std::memcpy(buf.data(), data + 1, size - 1);
    Bytes in(buf.data(), buf.size());
    std::size_t visited = 0;
    switch (op) {
    case 0: {
        auto row = std::make_unique<items::Row>();
        (void)items::read_definition(in, *row);
        break;
    }
    case 1: {
        auto row = std::make_unique<items::Row>();
        items::read_appearance(in, *row);
        break;
    }
    case 2: {
        std::size_t at = 0;
        (void)items::find_insertion_rules(in, at);
        break;
    }
    case 3: {
        // lane byte, 2-byte LE definition length, then definition followed by plug-set table.
        if (in.size() < 3) return -1;
        const auto lane = static_cast<std::uint8_t>(in[0]);
        std::size_t split = static_cast<std::size_t>(in[1]) | (static_cast<std::size_t>(in[2]) << 8);
        Bytes rest = in.subspan(3);
        if (split > rest.size()) split = rest.size();
        std::vector<std::byte> def(rest.begin(), rest.begin() + split);
        std::vector<std::byte> table(rest.begin() + split, rest.end());
        (void)items::visit_allowed_plugs(Bytes(def.data(), def.size()),
                                         Bytes(table.data(), table.size()), lane, &count_plug,
                                         &visited);
        break;
    }
    default:
        (void)items::visit_plug_set_flags(in, &count_flag, &visited);
        break;
    }
    return 0;
}
