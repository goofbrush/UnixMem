#include <sys/uio.h>
#include <signal.h>
#include <dirent.h>
#include <fstream>

class Memory {
public:
  Memory(const char* windowName, const char* executableName) { throw std::invalid_argument( "Dont use this constructor" ); }
  Memory(std::string procName) {
    pid = getProcIdByName(procName);
    getBase(procName);
  }

  bool checkValidity() {
    return kill(pid, 0)+1;
  }

  template <typename Type>
  Type readValue(uintptr_t address, bool based = false) { Type output;
    if(based) address+=BaseAddr;
    size_t bufferSize = sizeof(output);

    struct iovec local { &output,bufferSize };
    struct iovec remote { (void*)address,bufferSize };

    process_vm_readv(pid, &local, 1, &remote, 1, 0);
    return output;
  }

  uintptr_t readPointer(uintptr_t address, bool based = false) {
    return readValue<uintptr_t>(address,based);
  }

  template <typename Type>
  Type readValueFromPointer(uintptr_t address, bool based = false, uintptr_t localOffset = 0x0) {
    uintptr_t pointer = readPointer(address,based);
    return readValue<Type>(pointer+localOffset);
  }

  template <typename Type>
  void writeValue(Type input, uintptr_t address, bool based = false) {
    if(based) address+=BaseAddr;
    size_t bufferSize = sizeof(input);

    struct iovec local { &input,bufferSize };
    struct iovec remote { (void*)address,bufferSize };

    process_vm_writev(pid, &local, 1, &remote, 1, 0);
  }

  template <typename Type>
  void writeValueToPointer(Type input, uintptr_t address, bool based = false, uintptr_t localOffset = 0x0) {
    uintptr_t pointer = readPointer(address,based);
    writeValue<Type>(input, pointer+localOffset);
  }

  uintptr_t getase() { return BaseAddr; }

private:
  pid_t pid;                     // Process ID
  uintptr_t BaseAddr = 0x400000; // Base Address

  bool procNameLogic(std::string cmdLine, std::string procName) {
    return ((cmdLine.find(procName) != std::string::npos) &&
      (cmdLine.find("Z:\\") != std::string::npos));
    //return (cmdLine == procName);
  }
  int getProcIdByName(std::string procName){
    int pid = -1;
    // Open the /proc directory
    DIR *dp = opendir("/proc");
    if (dp != NULL) {
      // Enumerate all entries in directory until process found
      struct dirent *dirp;
      while (pid < 0 && (dirp = readdir(dp))) {
        // Skip non-numeric entries
        int id = atoi(dirp->d_name);
        if (id > 0) {
          // Read contents of virtual /proc/{pid}/cmdline file
          std::string cmdPath = std::string("/proc/") + dirp->d_name + "/cmdline";
          std::ifstream cmdFile(cmdPath.c_str());
          std::string cmdLine;
          getline(cmdFile, cmdLine);
          if (!cmdLine.empty()) {
            // Keep first cmdline item which contains the program path
            size_t pos = cmdLine.find('\0');
            if (pos != std::string::npos) { cmdLine = cmdLine.substr(0, pos); }
            // Keep program name only, removing the path
            pos = cmdLine.rfind('/');
            if (pos != std::string::npos) { cmdLine = cmdLine.substr(pos + 1); }
            // Compare against requested process name
            if(procNameLogic(cmdLine, procName)) { pid = id; }
          }
        }
      }
    }
    closedir(dp);
    return pid;
  }

  uintptr_t getBase(std::string name) {
    // "/proc/procID/maps"
    uintptr_t base = -1;
    std::string path = std::string("/proc/") + std::to_string(pid) + "/maps";
    std::ifstream file(path.c_str());
    std::string line;
    do {
      getline(file, line);
      if(line.find(name) != std::string::npos) {
        base = std::stoi(line.substr(0, line.find(":")));
      }
    }while(!line.empty());
    return base;
  }

};
