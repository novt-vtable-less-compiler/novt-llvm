void my_write(int fd, const char *data, size_t len) {
  if (fd < 0) return;
  while (len > 0) {
    auto count = write(fd, data, len);
    if (count < 0)
      return;
    data += count;
    len -= count;
  }
}

void ul_to_str(unsigned long value, char *ptr, int base = 10) {
  unsigned long t = 0, res = 0;
  unsigned long tmp = value;
  while (tmp > 0) {
    tmp = tmp / base;
  }

  do {
    res = value - base * (t = value / base);
    if (res < 10) {
      *--ptr = '0' + res;
    } else if ((res >= 10) && (res < 16)) {
      *--ptr = 'A' - 10 + res;
    }
  } while ((value = t) != 0);
}


static int open_file_output() {
  return open("/dev/shm/instrumentation.txt", O_WRONLY | O_APPEND | O_CREAT, 0664);
}


static inline char *write_header(int fd) {
  my_write(STDERR_FILENO, "=== INSTRUMENTATION ===\n", strlen("=== INSTRUMENTATION ===\n"));
  my_write(fd, "=== INSTRUMENTATION ===\n", strlen("=== INSTRUMENTATION ===\n"));
  char *buffer = (char *) my_malloc(16000);

  auto pos = strlen("METADATA: pid=");
  memmove(buffer, "METADATA: pid=", pos);
  for (int i = 0; i <= 16; i++) buffer[i + pos] = ' ';
  ul_to_str(getpid(), buffer + pos + 6);
  buffer[pos + 6] = '\n';
  my_write(STDERR_FILENO, buffer, pos + 7);
  my_write(fd, buffer, pos + 7);

  pos = strlen("CMDLINE: ");
  memmove(buffer, "CMDLINE: ", pos);
  auto fd2 = open("/proc/self/cmdline", O_RDONLY);
  if (fd2 > 0) {
    char buffer2[256];
    int count;
    while ((count = read(fd2, buffer2, sizeof(buffer2))) > 0) {
      for (int i = 0; i < count; i++)
        buffer[pos++] = buffer2[i] == '\0' ? ' ' : buffer2[i];
    }
    close(fd2);
    buffer[pos++] = '\n';
    my_write(STDERR_FILENO, buffer, pos);
    my_write(fd, buffer, pos);
  } else {
    my_write(STDERR_FILENO, "CMDLINE: error\n", strlen("CMDLINE: error\n"));
    my_write(fd, "CMDLINE: error\n", strlen("CMDLINE: error\n"));
  }
  return buffer;
}

static inline void write_count(int fd, char *buffer, const char *name, uint64_t count) {
  for (int i = 0; i <= 16; i++) buffer[i] = ' ';
  ul_to_str(count, buffer + 16);
  auto len = strlen(name);
  memmove(buffer + 17, name, len);
  buffer[17 + len] = '\n';
  auto size = 17 + len + 1;
  if (size > 0) {
    my_write(STDERR_FILENO, buffer, size);
    my_write(fd, buffer, size);
  }
}

static inline void write_counts(int fd, char *buffer, const char *name, volatile uint64_t counts[], size_t count_len) {
  for (int count_idx = 0; count_idx < count_len; count_idx++) {
    uint64_t cnt = counts[count_idx];
    if (cnt == 0) continue;

    for (int i = 0; i <= 16; i++) buffer[i] = ' ';
    ul_to_str(cnt, buffer + 16);
    auto len = strlen(name);
    memmove(buffer + 17, name, len);
    int pos = 17 + len;
    for (int i = 0; i <= 16; i++) buffer[pos + i] = ' ';
    pos += 16;
    ul_to_str(count_idx, buffer + pos);
    buffer[pos] = '\n';
    auto size = pos + 1;
    if (size > 0) {
      my_write(STDERR_FILENO, buffer, size);
      my_write(fd, buffer, size);
    }
  }
}
