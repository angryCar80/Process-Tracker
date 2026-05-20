#include <asm-generic/ioctls.h>
#include <cctype>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
// #include <sstream>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

struct termios orig_termios;

void disableRawMode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); }

void enableRawMode() {
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disableRawMode);
  struct termios raw = orig_termios;
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

class Proccess {
public:
  int pid;
  std::string name;
  Proccess(int pid, const std::string &name) : pid(pid), name(name) {}
};

std::string getProcessName(int pid) {
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d/comm", pid);

  FILE *fp = fopen(path, "r");
  if (!fp)
    return "unknown";

  char name[256];
  if (fgets(name, sizeof(name), fp)) {
    name[strcspn(name, "\n")] = 0; // strip newline
    fclose(fp);
    return std::string(name);
  }
  fclose(fp);
  return "unknown";
}

int main() {
  std::vector<Proccess> processes;
  // init the vector
  // 1. For loop of the system processes

  struct winsize w;
  ioctl(STDIN_FILENO, TIOCGWINSZ, &w);

  DIR *dir = opendir("/proc/");
  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (isdigit(entry->d_name[0])) {
      int pid = atoi(entry->d_name);
      std::string pname = getProcessName(pid);
      processes.emplace_back(pid, pname);
    }
  }
  closedir(dir);
  std::string input;

  int lineLength = 33;
  int padding = (w.ws_col - lineLength) / 2;

  std::cout << std::string(padding, ' ')
            << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  std::cout << std::string(padding, ' ') << "> ";
  std::cin >> input;

  return 0;
}
