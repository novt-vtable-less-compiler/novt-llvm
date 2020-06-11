// ../../cmake-build-minsizerel/bin/clang++ -c instrument.cpp -o instrument.bc -fno-rtti -fno-exceptions -nostdlib

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern "C" {
void __instrument_count(const char *name);
void __instrument_print();
}

namespace {

#include "my_spinlock.h"
#include "my_malloc.h"
#include "mylib.h"
#include "map.h"
#include "map.c"

    typedef map_t(uint64_t) uint64_map_t;
    uint64_map_t counters;

    void __attribute__((constructor)) __init() {
      map_init(&counters);
    }

    void __attribute__((destructor)) __deinit() {
      __instrument_print();
    }

}

void __instrument_count(const char *name) {
  Locker locker;
  auto element = map_get(&counters, name);
  if (element) (*element)++;
  else
    map_set(&counters, name, 1);
}

void __instrument_print() {
  Locker locker;
  char *buffer = write_header();

  const char *key;
  map_iter_t iter = map_iter(&counters);
  while ((key = map_next(&counters, &iter))) {
    // auto size = sprintf(buffer, "%15lu %s\n", *map_get(&counters, key), key);
    write_count(buffer, key, *map_get(&counters, key));
  }
  my_write("=== INSTRUMENTATION END ===\n", strlen("=== INSTRUMENTATION END ===\n"));
}

