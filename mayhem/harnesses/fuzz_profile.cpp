// Public profile exchange decoder (middleware/profile/public_profile_codec.h): the bytes a remote
// peer publishes as its profile. The whole input is the encoded body; `output` and `staging` are
// two distinct, fresh AccountState images per input, as the header requires.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <vector>

#include "middleware/profile/public_profile_codec.h"

namespace profile = sunrise::middleware::profile;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    std::vector<std::byte> buf(size);
    if (size) std::memcpy(buf.data(), data, size);
    auto output = std::make_unique<sunrise::state::AccountState>();
    auto staging = std::make_unique<sunrise::state::AccountState>();
    if (profile::decode(std::span<const std::byte>(buf.data(), buf.size()), *output, *staging)) {
        (void)profile::valid(*output);
    }
    return 0;
}
