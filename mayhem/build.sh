#!/usr/bin/env bash
#
# mayhem/build.sh — build Sunrise's fuzz harnesses and the known-answer selftest.
#
# Upstream builds one Windows DLL (MSVC / clang-cl + xwin) that cannot build on Linux. The wire
# codecs under Sunrise/src/middleware, core/json and state/mods/manifest are portable C++20, so this
# compiles exactly that portable set (mayhem/lib_sources.txt: every upstream .cpp that compiles on
# Linux) into a static library, once sanitized for the harnesses and once with normal flags for the
# selftest. Outputs go under mayhem/out (gitignored); fuzzers and reproducers land in /mayhem.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${SRC:=/mayhem}"
: "${AR:=llvm-ar}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"
SUN="$SRC/Sunrise/src"
OUT="$SRC/mayhem/out"
STD="-std=c++20"

# build_lib <name> <flags...>: compile every portable source into $OUT/<name>/lib.a
build_lib() {
  local name="$1"; shift
  local dir="$OUT/$name"
  mkdir -p "$dir/obj"
  printf '%s\n' "$@" > "$dir/flags.new"
  # Flags changed (e.g. a different SANITIZER_FLAGS) -> rebuild from scratch.
  if ! cmp -s "$dir/flags.new" "$dir/flags" 2>/dev/null; then rm -rf "$dir/obj"; mkdir -p "$dir/obj"; fi
  mv "$dir/flags.new" "$dir/flags"
  export LIBDIR="$dir" SUN CXX STD
  export LIBFLAGS="$*"
  grep -v '^\s*$' "$SRC/mayhem/lib_sources.txt" | xargs -P"$MAYHEM_JOBS" -I{} bash -c '
    f="{}"; o="$LIBDIR/obj/${f//\//__}.o"
    [ "$o" -nt "$SUN/$f" ] && exit 0
    $CXX $STD $LIBFLAGS -I"$SUN" -c "$SUN/$f" -o "$o"'
  # public_profiles.cpp (profiles::valid, which profile::decode calls) only needs <Windows.h> for
  # the SRWLOCK in core/threading/srw_lock.h; mayhem/compat/Windows.h supplies just that.
  local pp="$dir/obj/state__account__public_profiles.cpp.o"
  [ "$pp" -nt "$SUN/state/account/public_profiles.cpp" ] || \
    $CXX $STD $LIBFLAGS -I"$SUN" -I"$SRC/mayhem/compat" \
      -c "$SUN/state/account/public_profiles.cpp" -o "$pp"
  rm -f "$dir/lib.a"
  "$AR" rcs "$dir/lib.a" "$dir"/obj/*.o
}

# 1) Sanitized + instrumented library for the harnesses (the project code is what gets fuzzed).
FUZZ_FLAGS="$SANITIZER_FLAGS $DEBUG_FLAGS -O1 -fsanitize=fuzzer-no-link"
build_lib fuzz $FUZZ_FLAGS

$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$OUT/lsan_off.o"
# Logging sink stand-in (core/logging/log.cpp needs Windows.h); see the file's header.
$CXX $STD $SANITIZER_FLAGS $DEBUG_FLAGS -O1 -I"$SUN" -c "$SRC/mayhem/harnesses/log_stub.cpp" -o "$OUT/log_stub.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$OUT/standalone_main.o"

# 2) Harnesses: libFuzzer binary + standalone reproducer each.
for h in web_service bap_frame activity_message bap_services gameplay mod_manifest content_tables \
    item_definitions profile sense_update; do
  src="$SRC/mayhem/harnesses/fuzz_$h.cpp"
  $CXX $STD $SANITIZER_FLAGS $DEBUG_FLAGS -O1 -I"$SUN" -I"$SRC/mayhem/harnesses" -c "$src" \
    -o "$OUT/fuzz_$h.o"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$OUT/fuzz_$h.o" "$OUT/fuzz/lib.a" \
    "$OUT/log_stub.o" "$OUT/lsan_off.o" -o "$SRC/fuzz_$h"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS "$OUT/fuzz_$h.o" "$OUT/standalone_main.o" "$OUT/fuzz/lib.a" \
    "$OUT/log_stub.o" "$OUT/lsan_off.o" -o "$SRC/fuzz_$h-standalone"
done

# 3) Known-answer selftest with NORMAL flags (independent of the sanitized build) for test.sh.
build_lib plain -O1 $COVERAGE_FLAGS
$CXX $STD -O1 $COVERAGE_FLAGS -I"$SUN" "$SRC/mayhem/selftest.cpp" \
  "$SRC/mayhem/harnesses/log_stub.cpp" "$OUT/plain/lib.a" \
  -o "$OUT/sunrise_selftest"
echo "build.sh: done"
