//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

// A cache of fixed-size blocks, shared by all threads, with a fixed capacity and eviction, for readers that want a
// few bytes of a block at a time (a lookup into a compressed file: decompress the block once, read one value).
//
// Direct-mapped: block `id` lives in slot id % slots, so consecutive ids never collide and ids up to the number of
// slots are never evicted. Each slot has a sequence lock: a version (odd while a thread is loading a block into the
// slot) and the id it holds. A reader notes the version, checks the id, reads its value, and checks the version
// again; if a loader replaced the block meanwhile the read is repeated. So readers take no lock and write nothing
// shared: any number of threads read the same block at the same time. A thread that finds another block in the slot
// takes it (version odd), loads its own, and releases it (version even).
//
//   read(id, load, use)   use(const std::uint8_t* block) -> a value (trivially copyable); load(std::uint8_t* dst)
//                         fills dst with block id. `use` may run on a block that is being replaced: its result is
//                         then discarded, but it must stay within the block's bytes whatever they hold.
//
// The memory is anonymous and untouched until a slot is first loaded, so a cache larger than the data costs only the
// blocks read.

#include <sys/mman.h>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <thread>

template<std::size_t BlockBytes>
class SharedBlockCache
{
        struct alignas(64) Slot {
                std::atomic<std::uint64_t> version{ 0 };
                std::atomic<std::uint64_t> id{ ~std::uint64_t(0) };
        };
        std::size_t n_;
        std::unique_ptr<Slot[]> slots_;
        std::uint8_t* data_;
        std::atomic<std::uint64_t> loads_{ 0 };                 // blocks loaded (the misses)

public:
        explicit SharedBlockCache(std::uint64_t capacity_bytes)
        :
                n_(capacity_bytes / BlockBytes < 1 ? 1 : std::size_t(capacity_bytes / BlockBytes)),
                slots_(new Slot[n_])
        {
                void* const m = ::mmap(nullptr, n_ * BlockBytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
                if (m == MAP_FAILED) throw std::bad_alloc();
                data_ = static_cast<std::uint8_t*>(m);
        }
        ~SharedBlockCache() { ::munmap(data_, n_ * BlockBytes); }
        SharedBlockCache(SharedBlockCache const&) = delete;
        SharedBlockCache& operator=(SharedBlockCache const&) = delete;

        std::size_t slots() const { return n_; }
        std::uint64_t bytes() const { return std::uint64_t(n_) * BlockBytes; }
        std::uint64_t loads() const { return loads_.load(std::memory_order_relaxed); }

        template<class Load, class Use>
        auto read(std::uint64_t id, Load&& load, Use&& use)
        {
                Slot& s = slots_[id % n_];
                std::uint8_t* const block = data_ + (id % n_) * BlockBytes;
                for (;;) {
                        std::uint64_t v = s.version.load(std::memory_order_acquire);
                        if (v & 1) { std::this_thread::yield(); continue; }     // being loaded
                        if (s.id.load(std::memory_order_acquire) == id) {
                                auto const r = use(static_cast<std::uint8_t const*>(block));
                                if (s.version.load(std::memory_order_acquire) == v) return r;
                                continue;                                       // replaced while reading
                        }
                        if (!s.version.compare_exchange_strong(v, v + 1, std::memory_order_acquire)) continue;
                        load(block);
                        loads_.fetch_add(1, std::memory_order_relaxed);
                        s.id.store(id, std::memory_order_release);
                        s.version.store(v + 2, std::memory_order_release);
                }
        }
};
