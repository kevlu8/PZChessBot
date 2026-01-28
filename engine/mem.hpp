#include <cerrno>
#include <cstring>
#include <sys/mman.h>

#include <exception>
#include <string>

static void *large_alloc(size_t size) {
	void *ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	if (ptr == MAP_FAILED) {
		std::__throw_runtime_error(strerror(errno));
	}
	return ptr;
}

static void large_free(void *ptr, size_t size) {
	munmap(ptr, 0); // size is not needed for munmap
}
