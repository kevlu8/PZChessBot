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
	void *ptr = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (ptr == nullptr) {
		std::__throw_runtime_error(std::strerror(GetLastError()));
	}
	return ptr;
#else
	void *ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	if (ptr == MAP_FAILED) {
		std::__throw_runtime_error(strerror(errno));
	}

#ifdef MADV_HUGEPAGE
	madvise(ptr, size, MADV_HUGEPAGE);
#endif

	return ptr;
#endif
}

static void large_free(void *ptr, size_t size) {
#if defined(_WIN32)
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	munmap(ptr, size);
#endif
}
