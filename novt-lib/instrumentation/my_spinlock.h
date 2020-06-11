#ifndef LLVM_MY_SPINLOCK_H
#define LLVM_MY_SPINLOCK_H

static volatile int mylock = 0;

void lock(volatile int *exclusion) {
  while (__sync_lock_test_and_set(exclusion, 1))
    while (*exclusion) {}
}

void unlock(volatile int *exclusion) {
  __sync_synchronize(); // Memory barrier.
  *exclusion = 0;
}

struct Locker {
    Locker() {
      lock(&mylock);
    }

    ~Locker() {
      unlock(&mylock);
    }
};

#endif //LLVM_MY_SPINLOCK_H
