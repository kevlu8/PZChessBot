/*
 * PZChessBot, a UCI chess engine
 * Copyright (C) 2026 Kevin Lu and William Ma
 *
 * PZChessBot is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PZChessBot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with PZChessBot. If not, see <https://www.gnu.org/licenses/>.
 */

#include <cerrno>
#include <cstring>
#include <exception>
#include <string>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#endif

static void *large_alloc(size_t size) {
#if defined(_WIN32)
	void *ptr = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE | MEM_LARGE_PAGES, PAGE_READWRITE);
	if (ptr == nullptr) {
		ptr = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (ptr == nullptr)
			std::__throw_runtime_error("Could not allocate memory: " + GetLastError());
	}
#elif defined(__APPLE__)
	void *ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	if (ptr == MAP_FAILED) {
		std::__throw_runtime_error(strerror(errno));
	}
#elif defined(__linux__)
	void *ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	if (ptr == MAP_FAILED) {
		std::__throw_runtime_error(strerror(errno));
	}

	madvise(ptr, size, MADV_HUGEPAGE);
#else
#error Unsupported OS/kernel
#endif

	return ptr;
}

static void large_free(void *ptr, size_t size) {
#if defined(_WIN32)
	VirtualFree(ptr, 0, MEM_RELEASE);
#elif defined(__APPLE__) || defined(__linux__)
	munmap(ptr, size);
#else
#error Unsupported OS/kernel
#endif
}
