#ifndef LLVM_MY_MALLOC_H
#define LLVM_MY_MALLOC_H

#include <sys/mman.h>
#include <string.h>

static char *current_bucket = nullptr;
static size_t current_bucket_remaining_memory = 0;

static void new_bucket() {
  current_bucket = (char *) mmap(0, 8192, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (current_bucket == MAP_FAILED) {
    current_bucket = nullptr;
    return;
  }
  current_bucket_remaining_memory = 8192;
}

struct alloc_chunk {
    size_t size;
    size_t flags;
    char data[0];
};

void *my_malloc(size_t size) {
  if (size & 0xf)
    size = (size ^ (size & 0xf)) + 0x10;
  if (size >= 2048) {
    auto chunk = (alloc_chunk *) mmap(nullptr, size + sizeof(alloc_chunk), PROT_READ | PROT_WRITE,
                                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (chunk == MAP_FAILED)
      return nullptr;
    chunk->size = size;
    chunk->flags = 1;
    return chunk->data;
  }

  size_t capacity = size + sizeof(alloc_chunk);
  if (capacity > current_bucket_remaining_memory)
    new_bucket();
  auto chunk = (alloc_chunk *) current_bucket;
  current_bucket += capacity;
  current_bucket_remaining_memory -= capacity;
  chunk->size = size;
  chunk->flags = 2;
  return chunk->data;
}

void my_free(void *ptr) {
  if (!ptr) return;
  auto chunk = (alloc_chunk*) (((char*) ptr) - sizeof(alloc_chunk));
  if (chunk->flags == 1)
    munmap(chunk, chunk->size + sizeof(alloc_chunk));
}

void *my_realloc(void *ptr, size_t new_size) {
  if (!ptr)
    return my_malloc(new_size);
  auto chunk = (alloc_chunk*) (((char*) ptr) - sizeof(alloc_chunk));
  if (new_size <= chunk->size)
    return ptr;
  auto new_chunk_memory = my_malloc(new_size);
  memmove(new_chunk_memory, ptr, chunk->size);
  my_free(ptr);
  return new_chunk_memory;
}

#endif //LLVM_MY_MALLOC_H
