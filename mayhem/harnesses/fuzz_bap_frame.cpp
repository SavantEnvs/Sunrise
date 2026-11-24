// BAP TCP stream decoding: the listener's outer-frame scan (parse_stream_frame) over the bytes read
// off the socket, each frame's request payload, and the activity-message (svc-8) envelope a request
// body carries -- the path server/transport/bap_peer_session.cpp drives on untrusted TCP input.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "middleware/bap/activity_message/activity_message_request_parser.h"
#include "middleware/bap/frame.h"

namespace bap = sunrise::middleware::bap;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    std::vector<std::byte> buf(size);
    if (size) std::memcpy(buf.data(), data, size);
    std::span<const std::byte> rest(buf.data(), buf.size());

    // Whole-input decoders.
    bap::OuterFrame outer{};
    (void)bap::parse_frame(rest, outer);
    bap::RequestFrame plain{};
    (void)bap::parse_request(rest, plain);
    bap::ResponseFrame response{};
    (void)bap::parse_response_payload(rest, response);

    // The stream: consume frames one after another, as the session loop does.
    constexpr std::size_t kMaximumFrameSize = 1u << 20;
    while (!rest.empty()) {
        bap::OuterFrame frame{};
        std::size_t consumed = 0;
        if (bap::parse_stream_frame(rest, kMaximumFrameSize, frame, consumed) !=
                bap::StreamFrameResult::complete ||
            consumed == 0 || consumed > rest.size())
            break;
        bap::RequestFrame request{};
        if (bap::parse_request_payload(frame.payload, frame.frameType, request)) {
            bap::activity_message::Request activity{};
            (void)bap::activity_message::parse_request(request.body, activity);
        }
        rest = rest.subspan(consumed);
    }
    return 0;
}
