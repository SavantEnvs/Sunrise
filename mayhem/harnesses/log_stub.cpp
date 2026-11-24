// Linux stand-in for core/logging/log.cpp, which writes to a Windows log file (Windows.h) and so
// cannot build here. Only the logging sink is replaced: the codecs call accepts()/write() for
// diagnostics, and a harness has nowhere to log to. accepts() says no, so nothing is formatted.
#include "core/logging/log.h"

namespace sunrise::core::log {
bool accepts(Channel, Level) noexcept { return false; }
void write(Channel, Level, std::string_view) noexcept {}
void writef(Channel, Level, const char*, ...) noexcept {}
} // namespace sunrise::core::log
