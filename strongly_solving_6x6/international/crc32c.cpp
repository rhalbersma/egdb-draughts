//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// build: g++-14 -O2 -msse4.2 crc32c.cpp -o crc32c;  usage: crc32c file...
// CRC32C (Castagnoli) of files, with the SSE4.2 instruction
#include <cstdio>
#include <cstdint>
#include <nmmintrin.h>
#include <vector>
int main(int argc, char** argv) {
    for (int a = 1; a < argc; ++a) {
        std::FILE* f = std::fopen(argv[a], "rb"); if (!f) { std::printf("cannot open %s\n", argv[a]); continue; }
        std::vector<unsigned char> buf(1 << 24); std::uint64_t c = 0xFFFFFFFF, total = 0; std::size_t n;
        while ((n = std::fread(buf.data(), 1, buf.size(), f)) > 0) {
            std::size_t i = 0;
            for (; i + 8 <= n; i += 8) { std::uint64_t x; __builtin_memcpy(&x, &buf[i], 8); c = _mm_crc32_u64(c, x); }
            for (; i < n; ++i) c = _mm_crc32_u8(std::uint32_t(c), buf[i]);
            total += n;
        }
        std::fclose(f);
        std::printf("%08x %12llu %s\n", unsigned(c ^ 0xFFFFFFFF), (unsigned long long)total, argv[a]);
    }
}
