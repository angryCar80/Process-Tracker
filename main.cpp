// Defining Colors
#include <cstdio>
#include <sched.h>
#define Black "\e[0;30m"
#define Red "\e[0;31m"
#define Green "\e[0;32m"
#define Yellow "\e[0;33m"
#define Blue "\e[0;34m"
#define Purple "\e[0;35m"
#define Cyan "\e[0;36m"
#define White "\e[0;37m"
#define Reset "\033[0m"
#define Rev "\e[7m"
// Includes Nedded
#include <asm-generic/ioctls.h>
#include <cctype>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

// Terminal Managment function
struct termios orig_termios;

void disableRawMode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); }

void enableRawMode() {
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disableRawMode);
  struct termios raw = orig_termios;
  raw.c_lflag &=
      ~(ECHO | ICANON); // Terminal Flags to remove the echo of the input = ECHO
                        // and denying a key combo = ICANON
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Desktop App class for --drun mode
class DesktopApp {
public:
  std::string name;
  std::string exec;
};

// Getting Process and putting them in this class //
class Process {
public:
  int pid;
  std::string name;
  Process(int pid, const std::string &name) : pid(pid), name(name) {}
};

// To Get Process Name
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

std::string toLower(const std::string &s) {
  std::string r;
  for (char c : s)
    r += std::tolower(c);
  return r;
}

bool matches(const std::string &str, const std::string &q) {
  if (q.empty())
    return true;
  return toLower(str).find(toLower(q)) != std::string::npos;
}

std::vector<Process> filter(const std::vector<Process> &all,
                            const std::string &q) {
  std::vector<Process> res;
  for (const auto &p : all) {
    if (matches(p.name, q) || matches(std::to_string(p.pid), q)) {
      res.push_back(p);
    }
  }
  return res;
}

void clearScreen() { std::cout << "\033[2J\033[H"; }

std::string stripFieldCodes(const std::string &exec) {
  std::string r;
  for (size_t i = 0; i < exec.length(); i++) {
    if (exec[i] == '%' && i + 1 < exec.length() && std::isalpha(exec[i + 1])) {
      i++;
      continue;
    }
    r += exec[i];
  }
  while (!r.empty() && r.back() == ' ')
    r.pop_back();
  return r;
}

DesktopApp parseDesktopFile(const std::string &path) {
  FILE *fp = fopen(path.c_str(), "r");
  if (!fp)
    return {"", ""};

  char line[512];
  std::string name, exec;
  bool inEntry = false;
  bool noDisplay = false;
  bool hidden = false;

  while (fgets(line, sizeof(line), fp)) {
    if (line[0] == '#')
      continue;
    line[strcspn(line, "\n")] = 0;
    line[strcspn(line, "\r")] = 0;

    if (strcmp(line, "[Desktop Entry]") == 0) {
      inEntry = true;
      continue;
    }
    if (line[0] == '[' && line[strlen(line) - 1] == ']') {
      inEntry = false;
      continue;
    }
    if (!inEntry)
      continue;

    if (strncmp(line, "NoDisplay=", 10) == 0 && strcmp(line + 10, "true") == 0)
      noDisplay = true;
    if (strncmp(line, "Hidden=", 7) == 0 && strcmp(line + 7, "true") == 0)
      hidden = true;
    if (strncmp(line, "Type=", 5) == 0 && strcmp(line + 5, "Application") != 0)
      noDisplay = true;
    if (strncmp(line, "Name=", 5) == 0)
      name = line + 5;
    if (strncmp(line, "Exec=", 5) == 0)
      exec = stripFieldCodes(line + 5);
  }
  fclose(fp);

  if (name.empty() || exec.empty() || noDisplay || hidden)
    return {"", ""};
  return {name, exec};
}

std::vector<DesktopApp> scanDesktopDirs() {
  std::vector<DesktopApp> apps;
  const char *home = getenv("HOME");
  std::string userDir;
  if (home)
    userDir = std::string(home) + "/.local/share/applications/";

  const char *dirs[] = {"/usr/share/applications/",
                        "/usr/local/share/applications/",
                        userDir.empty() ? nullptr : userDir.c_str()};

  for (const char *d : dirs) {
    if (!d)
      continue;
    DIR *dir = opendir(d);
    if (!dir)
      continue;
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
      if (entry->d_type != DT_REG && entry->d_type != DT_LNK)
        continue;
      size_t len = strlen(entry->d_name);
      if (len < 8 || strcmp(entry->d_name + len - 8, ".desktop") != 0)
        continue;
      std::string full = std::string(d) + entry->d_name;
      DesktopApp app = parseDesktopFile(full);
      if (!app.name.empty())
        apps.push_back(app);
    }
    closedir(dir);
  }

  return apps;
}

void launchApp(const std::string &exec) {
  pid_t pid = fork();
  if (pid == 0) {
    setsid();
    int fd = open("/dev/null", O_RDWR);
    if (fd != -1) {
      dup2(fd, STDIN_FILENO);
      dup2(fd, STDOUT_FILENO);
      dup2(fd, STDERR_FILENO);
      if (fd > 2)
        close(fd);
    }
    execl("/bin/sh", "sh", "-c", exec.c_str(), NULL);
    exit(1);
  }
}

void printHeaderProcess(int pad) {
  std::cout << Cyan << std::string(pad, ' ')
            << "╔══════════════════════════════════════════════╗\n";
  std::cout << std::string(pad, ' ')
            << "║             Process Killer v1.0              ║\n";
  std::cout << std::string(pad, ' ')
            << "╚══════════════════════════════════════════════╝\n"
            << Reset;
  std::cout << "\n";
}
void printHeaderApp(int pad) {
  std::cout << Cyan << std::string(pad, ' ')
            << "╔══════════════════════════════════════════════╗\n";
  std::cout << std::string(pad, ' ')
            << "║               App Runner v1.0                ║\n";
  std::cout << std::string(pad, ' ')
            << "╚══════════════════════════════════════════════╝\n"
            << Reset;
  std::cout << "\n";
}

void printList(const std::vector<Process> &procs, int sel, int offset, int rows,
               int nw, int pad) {
  int end = offset + rows;
  if (end > (int)procs.size())
    end = procs.size();

  for (int i = offset; i < end; i++) {
    if (i == sel) {
      std::cout << std::string(pad, ' ') << Rev << Cyan << "▸ " << Reset << Rev
                << procs[i].name
                << std::string(nw - procs[i].name.length(), ' ') << Green
                << procs[i].pid << Reset "\n";
    } else {
      std::cout << std::string(pad, ' ') << "  " << procs[i].name
                << std::string(nw - procs[i].name.length(), ' ') << Green
                << procs[i].pid << Reset "\n";
    }
  }

  if (procs.size() > rows) {
    int showing = end - offset;
    std::cout << std::string(pad, ' ') << Purple << "─── " << offset + 1 << "-"
              << offset + showing << " of " << procs.size() << " ───\n"
              << Reset;
  }
}

int main(int argc, char **argv) {
  std::vector<Process> all;
  struct winsize w;
  ioctl(STDIN_FILENO, TIOCGWINSZ, &w);

  if (argc < 2) {

    DIR *dir = opendir("/proc/");
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
      if (isdigit(entry->d_name[0])) {
        int pid = atoi(entry->d_name);
        all.emplace_back(pid, getProcessName(pid));
      }
    }
    closedir(dir);

    int maxLen = 0;
    for (const auto &p : all)
      if (p.name.length() > maxLen)
        maxLen = p.name.length();
    int nw = maxLen + 2;
    int pad = (w.ws_col - (nw + 14)) / 2;
    if (pad < 0)
      pad = 0;

    enableRawMode();

    std::string q;
    int sel = 0;
    int offset = 0;
    int rows = w.ws_row - 9;
    if (rows < 5)
      rows = 5;
    bool listMode = false;

    while (true) {
      clearScreen();
      printHeaderProcess(pad);

      auto res = filter(all, q);
      if (!res.empty()) {
        if (sel >= (int)res.size())
          sel = res.size() - 1;
        if (sel < 0)
          sel = 0;
      }

      int maxOffset = (int)res.size() - rows;
      if (maxOffset < 0)
        maxOffset = 0;
      if (offset > maxOffset)
        offset = maxOffset;
      if (offset < 0)
        offset = 0;
      if (sel < offset)
        offset = sel;
      if (sel >= offset + rows && rows > 0)
        offset = sel - rows + 1;

      if (listMode) {
        std::cout << std::string(pad, ' ') << White << "Search" << Green << ": "
                  << Yellow << q << Reset "\n\n";
      } else {
        std::cout << std::string(pad, ' ') << White << "Search" << Green << ": "
                  << Reset << q << Cyan << "█" << Reset "\n\n";
      }

      std::cout << std::string(pad, ' ') << White << "Processes (" << Green
                << res.size() << White << ")" << Reset "\n";
      printList(res, listMode ? sel : -1, offset, rows, nw, pad);

      if (listMode) {
        std::cout << "\n"
                  << std::string(pad, ' ') << Yellow
                  << "j/k/↑↓ scroll  |  Enter kill  |  Tab search  |  q quit"
                  << Reset "\n";
      } else {
        std::cout << "\n"
                  << std::string(pad, ' ') << Yellow
                  << "Type to filter  |  Tab browse  |  q quit" << Reset "\n";
      }
      std::cout << std::flush;

      char c;
      read(STDIN_FILENO, &c, 1);

      if (c == '\t') {
        listMode = !listMode;
        if (!listMode)
          sel = 0;
      } else if (c == 'q') {
        break;
      } else if (c == '\x1b') {
        char seq[2];
        if (read(STDIN_FILENO, &seq[0], 1) == 1 &&
            read(STDIN_FILENO, &seq[1], 1) == 1) {
          if (seq[0] == '[') {
            if (seq[1] == 'A' && listMode && !res.empty())
              sel--;
            else if (seq[1] == 'B' && listMode && !res.empty())
              sel++;
          }
        }
      } else if (c == 'j' && listMode && !res.empty()) {
        sel++;
      } else if (c == 'k' && listMode && !res.empty()) {
        sel--;
      } else if ((c == '\n' || c == '\r') && listMode && !res.empty()) {
        clearScreen();
        std::cout << Red << "\nKill " << res[sel].name << " (" << res[sel].pid
                  << ")? (y/N) " << Reset << std::flush;
        char yn;
        read(STDIN_FILENO, &yn, 1);
        if (yn == 'y' || yn == 'Y') {
          if (kill(res[sel].pid, SIGTERM) == 0) {
            std::cout << Green << "Sent SIGTERM \u2713\n" << Reset;
          } else {
            std::cout << Red << "Failed \u2717\n" << Reset;
          }
        } else {
          std::cout << Yellow << "Cancelled\n" << Reset;
        }
        std::cout << "Press any key..." << std::flush;
        read(STDIN_FILENO, &c, 1);
      } else if ((c == '\x7f' || c == '\x08') && !listMode && !q.empty()) {
        q.pop_back();
      } else if (!listMode && c >= 32 && c <= 126) {
        q += c;
      }
    }

    disableRawMode();
    clearScreen();
    std::cout << Green << "Bye!" << Reset "\n";
  }
  if (argc >= 2 && strcmp("--drun", argv[1]) == 0) {
    auto apps = scanDesktopDirs();

    int maxLen = 0;
    for (const auto &a : apps)
      if (a.name.length() > maxLen)
        maxLen = a.name.length();
    int nw = maxLen + 2;
    int pad = (w.ws_col - (nw + 14)) / 2;
    if (pad < 0)
      pad = 0;
    int rows = w.ws_row - 9;
    if (rows < 5)
      rows = 5;

    enableRawMode();

    std::string q;
    int sel = 0;
    int offset = 0;

    while (true) {
      clearScreen();
      printHeaderApp(pad);

      std::vector<DesktopApp> res;
      for (const auto &a : apps)
        if (matches(a.name, q))
          res.push_back(a);

      if (!res.empty()) {
        if (sel >= (int)res.size())
          sel = res.size() - 1;
        if (sel < 0)
          sel = 0;
      }

      int maxOffset = (int)res.size() - rows;
      if (maxOffset < 0)
        maxOffset = 0;
      if (offset > maxOffset)
        offset = maxOffset;
      if (offset < 0)
        offset = 0;
      if (sel < offset)
        offset = sel;
      if (sel >= offset + rows && rows > 0)
        offset = sel - rows + 1;

      std::cout << std::string(pad, ' ') << White << "Search" << Green << ": "
                << Reset << q << Cyan << "█" << Reset "\n\n";

      std::cout << std::string(pad, ' ') << White << "Apps (" << Green
                << res.size() << White << ")" << Reset "\n";

      int end = offset + rows;
      if (end > (int)res.size())
        end = res.size();
      for (int i = offset; i < end; i++) {
        if (i == sel) {
          std::cout << std::string(pad, ' ') << Rev << Cyan << "▸ " << Reset
                    << Rev << res[i].name
                    << std::string(nw - res[i].name.length(), ' ') << Green
                    << "launch" << Reset "\n";
        } else {
          std::cout << std::string(pad, ' ') << "  " << res[i].name
                    << std::string(nw - res[i].name.length(), ' ') << Green
                    << "launch" << Reset "\n";
        }
      }
      if (res.size() > rows) {
        int showing = end - offset;
        std::cout << std::string(pad, ' ') << Purple << "─── " << offset + 1
                  << "-" << offset + showing << " of " << res.size() << " ───\n"
                  << Reset;
      }

      std::cout << "\n"
                << std::string(pad, ' ') << Yellow
                << "Type to filter  |  Enter launch  |  q quit" << Reset "\n";
      std::cout << std::flush;

      char c;
      read(STDIN_FILENO, &c, 1);

      if (c == 'q') {
        break;
      } else if (c == '\x1b') {
        char seq[2];
        if (read(STDIN_FILENO, &seq[0], 1) == 1 &&
            read(STDIN_FILENO, &seq[1], 1) == 1) {
          if (seq[0] == '[') {
            if (seq[1] == 'A' && !res.empty())
              sel--;
            else if (seq[1] == 'B' && !res.empty())
              sel++;
          }
        }
      } else if (c == 'j' && !res.empty()) {
        sel++;
      } else if (c == 'k' && !res.empty()) {
        sel--;
      } else if ((c == '\n' || c == '\r') && !res.empty()) {
        disableRawMode();
        launchApp(res[sel].exec);
        clearScreen();
        return 0;
      } else if (c == '\x7f' || c == '\x08') {
        if (!q.empty())
          q.pop_back();
      } else if (c >= 32 && c <= 126) {
        q += c;
      }
    }

    disableRawMode();
    clearScreen();
    std::cout << Green << "Bye!" << Reset "\n";
    return 0;
  }
  return 0;
}
