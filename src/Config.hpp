#include <arpa/inet.h>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace Config {
    constexpr const char* IP_ADDRESS = "127.0.0.1";
    constexpr int PORT = 8080;
    constexpr int THREAD_COUNT = 4;
    constexpr int BUFFER_SIZE = 1461;
    constexpr const char* FILE_NUMBERS = "numbers.txt"; 
    constexpr const char* FILE_STRINGS =  "strings.txt";
    constexpr char SPLITERATOR = ',';
}