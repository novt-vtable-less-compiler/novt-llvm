// ../../cmake-build-minsizerel/bin/clang++ -c instrument-lite.cpp -o instrument-lite.bc -fno-rtti -fno-exceptions -nostdlib
// ../../cmake-build-minsizerel/bin/clang++ -shared instrument-lite.cpp -o libinstrument-lite.so -O3 -fno-rtti -fno-exceptions -no-novt-libs -lpthread -nostdlib++ -fPIC

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>

extern "C" {
void __instrument_count_lite(unsigned int type);
void __instrument_print();
}

namespace {

#include "my_malloc.h"
#include "mylib.h"

    volatile uint64_t counters[10];
    bool has_printed = false;

    void clear_counters() {
      // fprintf(stderr, "Fork: %d\n", getpid());
      for (int i = 0; i < 10; i++) counters[i] = 0;
    }

    void my_signal_handler(int signal) {
      // fprintf(stderr, "Handled signal %d in pid %d\n", signal, getpid());
      _exit(signal);
    }

    void __attribute__((constructor)) __init() {
      clear_counters();
      pthread_atfork(nullptr, nullptr, clear_counters);
      // fprintf(stderr, "INIT: %d\n", getpid());
      signal(SIGTERM, my_signal_handler);
      signal(SIGUSR2, my_signal_handler);
    }

    void __attribute__((destructor)) __deinit() {
      __instrument_print();
    }


}

void __instrument_count_lite(unsigned int type) {
  // counters[type]++;
  __sync_add_and_fetch(counters + type, 1);
}

void __instrument_print() {
  int fd = open_file_output();
  char *buffer = write_header(fd);
  write_count(fd, buffer, "dispatcher", counters[0]);
  write_count(fd, buffer, "vbase", counters[1]);
  write_count(fd, buffer, "dynamic_cast", counters[2]);
  write_count(fd, buffer, "dynamic_cast_void", counters[3]);
  write_count(fd, buffer, "rtti", counters[4]);
  my_write(STDERR_FILENO, "=== INSTRUMENTATION END ===\n", strlen("=== INSTRUMENTATION END ===\n"));
  my_write(fd, "=== INSTRUMENTATION END ===\n", strlen("=== INSTRUMENTATION END ===\n"));
  if (fd >= 0) close(fd);
  has_printed = true;
}



// ================================================================================================


void _exit (int status) {
  if (!has_printed) {
    // fprintf(stderr, "Trigger print from _exit(%d) in %d\n", status, getpid());
    __instrument_print();
  }
  // fprintf(stderr, "Trigger exit from _exit(%d) in %d\n", status, getpid());
  syscall(231, status); // EXIT_GROUP
  syscall(60, status); // EXIT
}

//real_function(kill, int(*)(pid_t, int));
int kill(pid_t pid, int sig) {
  // fprintf(stderr, "kill(%d, %d) from %d\n", pid, sig, getpid());
  if (sig == SIGKILL || sig == SIGTERM) {
    int result1 = syscall(62, pid, SIGUSR2);
    usleep(200000);
    // fprintf(stderr, "kill(%d, %d) from %d, this time for real\n", pid, sig, getpid());
    int result2 = syscall(62, pid, sig);
    errno = result1 == 0 ? result1 : result2;
  } else {
    errno = syscall(62, pid, sig);
  }
  return errno == 0 ? 0 : -1;
}
