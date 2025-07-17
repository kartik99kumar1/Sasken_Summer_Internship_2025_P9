#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstdio>

#define PORT 8080
#define PASSWORD "admin123"  // Simple authentication password

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    char buffer[1024] = {0};
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket file descriptor (IPv4, TCP)
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Attach socket to port
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Define server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Accept any incoming IP
    address.sin_port = htons(PORT);

    // Bind socket to the address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << "...\n";

    // Accept incoming connection
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Accept");
        exit(EXIT_FAILURE);
    }

    // Step 1: Authenticate
    read(new_socket, buffer, 1024);
    if (strcmp(buffer, PASSWORD) != 0) {
        send(new_socket, "Authentication Failed", strlen("Authentication Failed"), 0);
        std::cout << "Authentication Failed\n";
        close(new_socket);
        return 0;
    } else {
        send(new_socket, "Authentication Successful", strlen("Authentication Successful"), 0);
        std::cout << "Client Authenticated\n";
    }

    // Step 2: Receive commands and execute
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        read(new_socket, buffer, 1024);

        if (strcmp(buffer, "exit") == 0) {
            std::cout << "Client disconnected.\n";
            break;
        }

        std::cout << "Executing Command: " << buffer << std::endl;

        // Run command and capture output
        FILE *fp = popen(buffer, "r");
        if (fp == NULL) {
            const char* error = "Failed to run command";
            send(new_socket, error, strlen(error), 0);
            continue;
        }

        char output[4096] = {0};
        fread(output, sizeof(char), sizeof(output) - 1, fp);
        pclose(fp);

        // Send output back to client
        send(new_socket, output, strlen(output), 0);
    }

    close(new_socket);
    close(server_fd);
    return 0;
}
