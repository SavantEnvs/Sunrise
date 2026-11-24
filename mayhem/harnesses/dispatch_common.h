// Shared helpers for the generated dispatch harnesses (see mayhem/gen_dispatch.py).
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <vector>

namespace mayhem_dispatch {
using Bytes = std::span<const std::byte>;
struct Entry {
    const char* name;
    bool (*run)(Bytes);
};
// Output structs can be large fixed-capacity arrays: heap-allocate one, value-initialized, per call.
template <class T> bool run_v(bool (*f)(Bytes) noexcept, Bytes in) { return f(in); }
template <class T> bool run_o(bool (*f)(Bytes, T&) noexcept, Bytes in) {
    auto out = std::make_unique<T>();
    return f(in, *out);
}
template <class T> bool run_oc(bool (*f)(Bytes, T&, std::size_t&) noexcept, Bytes in) {
    auto out = std::make_unique<T>();
    std::size_t consumed = 0;
    return f(in, *out, consumed);
}
template <class T> bool run_bo(bool (*f)(Bytes, std::size_t, T&) noexcept, Bytes in) {
    auto out = std::make_unique<T>();
    return f(in, in.size() * 8, *out);
}
inline bool run_v(bool (*f)(Bytes) noexcept, Bytes in) { return f(in); }

// The decoders read only the span they are given, so the payload is copied into an exactly-sized
// heap buffer: ASan then sees any read past the declared end.
inline int dispatch(const Entry* entries, std::size_t count, const uint8_t* data, size_t size) {
    if (size < 1 || count == 0) return -1;
    const Entry& e = entries[data[0] % count];
    std::vector<std::byte> buf(size - 1);
    if (size > 1) std::memcpy(buf.data(), data + 1, size - 1);
    (void)e.run(Bytes(buf.data(), buf.size()));
    return 0;
}
} // namespace mayhem_dispatch

#include "middleware/encoding/bit_reader.h"
namespace mayhem_dispatch {
using BitReader = sunrise::middleware::encoding::bits::Reader;
// Bit-stream readers: a fresh Reader over the whole span, as the packet decoders construct one.
template <class T> bool run_r(bool (*f)(BitReader&, T&) noexcept, Bytes in) {
    auto out = std::make_unique<T>();
    BitReader reader(in);
    return f(reader, *out);
}
inline bool run_rv(bool (*f)(BitReader&) noexcept, Bytes in) {
    BitReader reader(in);
    return f(reader);
}
} // namespace mayhem_dispatch
namespace mayhem_dispatch {
// (span, bool flag, T&): decode both ways (the flag selects an optional section of the grammar).
template <class T> bool run_sbo(bool (*f)(Bytes, bool, T&) noexcept, Bytes in) {
    auto out = std::make_unique<T>();
    bool a = f(in, false, *out);
    auto out2 = std::make_unique<T>();
    return f(in, true, *out2) || a;
}
} // namespace mayhem_dispatch
