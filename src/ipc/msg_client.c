// l-shell-msg: the IPC client without the shell. `l-shell msg` loads the whole shell binary and every library it
// links (cairo, pango, PipeWire, D-Bus, …) just to write one line to a socket; bound to a key, that start-up cost
// is most of the delay before Spotlight appears. This client links only libc. Same protocol as IpcClient::send.

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

#ifndef NOCTALIA_APP_NAME
#define NOCTALIA_APP_NAME "l-shell"
#endif

// Separates the caller's working directory from the command (kCallerCwdSeparator in ipc_client.cpp).
static const char kCwdSeparator = '\x1e';

static int writeAll(int fd, const char* data, size_t size) {
  while (size > 0) {
    const ssize_t n = write(fd, data, size);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
    data += n;
    size -= (size_t)n;
  }
  return 0;
}

int main(int argc, char* argv[]) {
  if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
    fputs("usage: l-shell-msg <command> [args...]   (same commands as `l-shell msg`)\n", stderr);
    return argc < 2 ? 1 : 0;
  }
  // notification-show packs its text as JSON; leave that to the full client.
  if (strcmp(argv[1], "notification-show") == 0) {
    char** args = calloc((size_t)argc + 2, sizeof(char*));
    if (args == NULL) {
      return 1;
    }
    args[0] = NOCTALIA_APP_NAME;
    args[1] = "msg";
    for (int i = 1; i < argc; ++i) {
      args[i + 1] = argv[i];
    }
    execvp(NOCTALIA_APP_NAME, args);
    perror("error: exec " NOCTALIA_APP_NAME);
    return 1;
  }

  const char* runtime = getenv("XDG_RUNTIME_DIR");
  if (runtime == NULL || runtime[0] == '\0') {
    runtime = "/tmp";
  }
  const char* display = getenv("WAYLAND_DISPLAY");
  if (display == NULL || display[0] == '\0') {
    display = "wayland-0";
  }
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  const int pathLen =
      snprintf(addr.sun_path, sizeof(addr.sun_path), "%s/" NOCTALIA_APP_NAME "-%s.sock", runtime, display);
  if (pathLen < 0 || (size_t)pathLen >= sizeof(addr.sun_path)) {
    fputs("error: socket path too long\n", stderr);
    return 1;
  }

  const int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (fd < 0) {
    perror("error: socket");
    return 1;
  }
  const struct timeval timeout = {.tv_sec = 2, .tv_usec = 0};
  setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  if (connect(fd, (const struct sockaddr*)&addr, sizeof(addr)) < 0) {
    fputs("error: " NOCTALIA_APP_NAME " is not running\n", stderr);
    close(fd);
    return 1;
  }

  // "<cwd>\x1e<command> <args...>", written in pieces; the daemon reads until end of stream.
  char cwd[PATH_MAX];
  int failed = 0;
  if (getcwd(cwd, sizeof(cwd)) != NULL && cwd[0] == '/') {
    failed |= writeAll(fd, cwd, strlen(cwd));
    failed |= writeAll(fd, &kCwdSeparator, 1);
  }
  for (int i = 1; i < argc && !failed; ++i) {
    if (i > 1) {
      failed |= writeAll(fd, " ", 1);
    }
    failed |= writeAll(fd, argv[i], strlen(argv[i]));
  }
  if (failed || shutdown(fd, SHUT_WR) < 0) {
    perror("error: write");
    close(fd);
    return 1;
  }

  // Echo the response; an "error:" reply fails the command, like `l-shell msg`.
  char buf[4096];
  int first = 1;
  int isError = 0;
  for (;;) {
    const ssize_t n = read(fd, buf, sizeof(buf));
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      perror("error: read");
      close(fd);
      return 1;
    }
    if (n == 0) {
      break;
    }
    if (first) {
      isError = n >= 6 && memcmp(buf, "error:", 6) == 0;
      first = 0;
    }
    fwrite(buf, 1, (size_t)n, stdout);
  }
  close(fd);
  return isError ? 1 : 0;
}
