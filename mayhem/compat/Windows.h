// Linux stand-in for the one piece of <Windows.h> that core/threading/srw_lock.h uses: SRWLOCK
// and its acquire/release calls, as a small constexpr-initializable reader/writer spinlock (the
// real SRWLOCK is a pointer-sized word set to SRWLOCK_INIT). Used only on the compile line of
// state/account/public_profiles.cpp (see mayhem/build.sh), which profile::decode links against
// for profiles::valid(); nothing else in the library sees this header.
#pragma once
#include <atomic>
#include <thread>

struct SRWLOCK {
    std::atomic<long> state; // -1: held exclusive, n > 0: n shared holders
};
typedef SRWLOCK* PSRWLOCK;
#define SRWLOCK_INIT {0}
inline bool TryAcquireSRWLockExclusive(PSRWLOCK s) {
    long expected = 0;
    return s->state.compare_exchange_strong(expected, -1, std::memory_order_acquire);
}
inline bool TryAcquireSRWLockShared(PSRWLOCK s) {
    long v = s->state.load(std::memory_order_relaxed);
    return v >= 0 && s->state.compare_exchange_strong(v, v + 1, std::memory_order_acquire);
}
inline void AcquireSRWLockExclusive(PSRWLOCK s) { while (!TryAcquireSRWLockExclusive(s)) std::this_thread::yield(); }
inline void AcquireSRWLockShared(PSRWLOCK s) { while (!TryAcquireSRWLockShared(s)) std::this_thread::yield(); }
inline void ReleaseSRWLockExclusive(PSRWLOCK s) { s->state.store(0, std::memory_order_release); }
inline void ReleaseSRWLockShared(PSRWLOCK s) { s->state.fetch_sub(1, std::memory_order_release); }
