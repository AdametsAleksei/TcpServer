#include "Config.hpp"

using namespace Config;

class ClientHandler {
  int clientSocket;
  std::mutex &numMutex;
  std::mutex &strMutex;
  std::ofstream &numbersFile;
  std::ofstream &stringsFile;

public:
  ClientHandler(int clientSocket, std::mutex &numMutex, std::mutex &strMutex,
                std::ofstream &numFile, std::ofstream &strFile)
      : clientSocket(clientSocket), numMutex(numMutex), strMutex(strMutex),
        numbersFile(numFile), stringsFile(strFile) {}

  void handle() {
    char buffer[BUFFER_SIZE];
    ssize_t bytesReceived;
    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) >
           0) {
      buffer[bytesReceived] = 0;
      std::istringstream stream(buffer);
      std::string currLine;
      while (std::getline(stream, currLine, SPLITERATOR)) {
        if (!currLine.empty()) {
          if (isNumber(currLine)) {
            std::scoped_lock<std::mutex> lock(numMutex);
            numbersFile << currLine << std::endl;
          } else {
            std::scoped_lock<std::mutex> lock(strMutex);
            stringsFile << currLine << std::endl;
          }
        }
      }
      std::cout << "Processed: " << buffer << std::endl;
    }
  }

private:
  static bool isNumber(const std::string &s) {
    for (char c : s) {
      if (!isdigit(c))
        return false;
    }
    return !s.empty();
  }
};

class Server {
  int port;
  int serverSocket = -1;
  std::mutex numMutex;
  std::mutex strMutex;

public:
  explicit Server(int port) : port(port) {}

  ~Server() {
    if (serverSocket >= 0) {
      close(serverSocket);
    }
  }

  void run() {
    setupSocket();
    bindAndListen();
    acceptClients();
  }

private:
  void setupSocket() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
      throw std::runtime_error("Failed to create socket");
    }
  }

  void bindAndListen() {
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, IP_ADDRESS, &serverAddr.sin_addr) <= 0) {
      throw std::runtime_error("Invalid address format");
    }

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) <
        0) {
      throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverSocket, 10) < 0) {
      throw std::runtime_error("Failed to listen on socket");
    }

    std::cout << "Server listening on port " << port << std::endl;
  }

  void openOutputFiles(std::ofstream &numFile, std::ofstream &strFile) {
    numFile.open(FILE_NUMBERS, std::ios::app);
    strFile.open(FILE_STRINGS, std::ios::app);

    if (!numFile || !strFile) {
      throw std::runtime_error("Failed to open output files");
    }
  }

  void acceptClients() {
    std::ofstream numFile, strFile;
    openOutputFiles(numFile, strFile);

    auto acceptLoop = [this, &numFile, &strFile]() {
      while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket =
            accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSocket >= 0) {
          std::cout << "New client connected: "
                    << inet_ntoa(clientAddr.sin_addr) << std::endl;
          std::jthread clientThread([this, clientSocket, &numFile, &strFile]() {
            ClientHandler handler(clientSocket, numMutex, strMutex, numFile,
                                  strFile);
            handler.handle();
          });
        } else {
          std::cerr << "Failed to accept client\n";
        }
      }
    };

    std::vector<std::jthread> acceptThreads;
    for (int i = 0; i < THREAD_COUNT; ++i) {
      acceptThreads.emplace_back(acceptLoop);
    }
  }
};

int main() {
  try {
    Server server(PORT);
    server.run();
  } catch (const std::exception &e) {
    std::cerr << "Server error: " << e.what() << std::endl;
  }

  return 0;
}