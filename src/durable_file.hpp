//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

// Durable files: the only platform-specific code of the pipeline. The standard library can hand data to the operating
// system (std::fflush) but not make it reach the disk, nor make a rename survive a power loss.
//
//   sync_file(f)               flush f and wait until its data is on disk (POSIX fsync, Windows _commit)
//   durable_rename(from, to)   replace `to` by `from` (also if `to` exists) and wait until the rename is on disk
//                              (POSIX rename + fsync of the directory, Windows MoveFileExW with write-through)
//
// Usage: write a temporary file, sync_file it, close it, durable_rename it into place. A file then either has its old
// contents or its complete new contents, also after a crash.

#include <cstdio>
#include <filesystem>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <io.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

inline bool sync_file(std::FILE* f)
{
        if (std::fflush(f) != 0) return false;
#ifdef _WIN32
        return ::_commit(::_fileno(f)) == 0;
#else
        return ::fsync(::fileno(f)) == 0;
#endif
}

inline bool durable_rename(std::string const& from, std::string const& to)
{
#ifdef _WIN32
        std::filesystem::path const a(from), b(to);
        return ::MoveFileExW(a.c_str(), b.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
        if (std::rename(from.c_str(), to.c_str()) != 0) return false;
        std::filesystem::path dir = std::filesystem::path(to).parent_path();
        if (dir.empty()) dir = ".";
        int const fd = ::open(dir.c_str(), O_RDONLY | O_DIRECTORY);
        if (fd < 0) return false;
        bool const ok = ::fsync(fd) == 0;
        return ::close(fd) == 0 && ok;
#endif
}
