#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstdio>
#include <pthread.h>

#define PORT 8080
#define PASSWORD "admin123"

// Function prototype
void* handle_client(void* socket_desc);

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Define address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 10) < 0) {
        perror("Listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << "...\n";

    while (true) {
        // Accept new connection
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept");
            exit(EXIT_FAILURE);
        }

        std::cout << "New client connected.\n";

        // Create thread for each client
        pthread_t thread_id;
        int* new_sock = new int;
        *new_sock = new_socket;

        if (pthread_create(&thread_id, NULL, handle_client, (void*)new_sock) < 0) {
            perror("Could not create thread");
            delete new_sock;
            continue;
        }

        pthread_detach(thread_id); // Clean up automatically when thread ends
    }

    close(server_fd);
    return 0;
}

// ======================
// Client Handler Function
// ======================
void* handle_client(void* socket_desc) {
    int sock = *(int*)socket_desc;
    delete (int*)socket_desc; // Free the allocated memory

    char buffer[4096] = {0};

    // Step 1: Authentication
    read(sock, buffer, 1024);
    if (strcmp(buffer, PASSWORD) != 0) {
        send(sock, "Authentication Failed", strlen("Authentication Failed"), 0);
        std::cout << "Authentication Failed for client.\n";
        close(sock);
        pthread_exit(NULL);
    } else {
        send(sock, "Authentication Successful", strlen("Authentication Successful"), 0);
        std::cout << "Client Authenticated.\n";
    }

    // Step 2: Command Handling Loop
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t read_size = read(sock, buffer, 1024);

        if (read_size <= 0) {
            std::cout << "Client disconnected.\n";
            break;
        }

        if (strcmp(buffer, "exit") == 0) {
            std::cout << "Client requested to exit.\n";
            break;
        }

        std::cout << "Executing command: " << buffer << std::endl;

        // Execute command
        FILE* fp = popen(buffer, "r");
        if (fp == NULL) {
            const char* error = "Failed to run command";
            send(sock, error, strlen(error), 0);
            continue;
        }

        char output[4096] = {0};
        fread(output, sizeof(char), sizeof(output) - 1, fp);
        pclose(fp);

        // Send output back
        send(sock, output, strlen(output), 0);
    }

    close(sock);
    pthread_exit(NULL);
}
