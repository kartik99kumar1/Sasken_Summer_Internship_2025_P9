#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>

#define PORT 8080

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[4096] = {0};
    std::string input;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "Socket creation error\n";
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Server IP address (Change if server is on another machine)
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        std::cout << "Invalid address/ Address not supported \n";
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "Connection Failed \n";
        return -1;
    }

    // Step 1: Send Password
    std::cout << "Enter Password: ";
    std::getline(std::cin, input);
    send(sock, input.c_str(), input.length(), 0);

    read(sock, buffer, 1024);
    std::cout << buffer << std::endl;
    if (std::string(buffer).find("Failed") != std::string::npos) {
        close(sock);
        return 0;
    }

    // Step 2: Send commands
    while (true) {
        std::cout << "Enter Command (or 'exit' to quit): ";
        std::getline(std::cin, input);

        send(sock, input.c_str(), input.length(), 0);

        if (input == "exit") {
            break;
        }

        memset(buffer, 0, sizeof(buffer));
        read(sock, buffer, 4096);
        std::cout << "Output:\n" << buffer << std::endl;
    }

    close(sock);
    return 0;
}
