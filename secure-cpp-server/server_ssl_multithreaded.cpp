#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstdio>
#include <pthread.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 8080
#define PASSWORD "admin123"

void* handle_client(void* ssl_ptr);

int main() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    const SSL_METHOD* method = TLS_server_method();
    SSL_CTX* ctx = SSL_CTX_new(method);

    if (!ctx) {
        std::cerr << "Unable to create SSL context" << std::endl;
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    // Load cert and key
    if (SSL_CTX_use_certificate_file(ctx, "certs/cert.pem", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, "certs/key.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "🔐 SSL Server listening on port " << PORT << "..." << std::endl;

    while (true) {
        int client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (client_fd < 0) {
            perror("Accept");
            continue;
        }

        std::cout << "📥 Client connected." << std::endl;

        SSL* ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client_fd);

        if (SSL_accept(ssl) <= 0) {
            std::cerr << "❌ SSL handshake failed" << std::endl;
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(client_fd);
            continue;
        } else {
            std::cout << "✅ SSL handshake successful" << std::endl;
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void*)ssl) != 0) {
            perror("Thread creation failed");
            SSL_free(ssl);
            close(client_fd);
        }
        pthread_detach(thread_id);
    }

    close(server_fd);
    SSL_CTX_free(ctx);
    EVP_cleanup();
    return 0;
}

void* handle_client(void* ssl_ptr) {
    SSL* ssl = (SSL*)ssl_ptr;
    char buffer[4096] = {0};

    std::cout << "🔑 Authenticating client..." << std::endl;

    // Step 1: Authentication
    SSL_read(ssl, buffer, sizeof(buffer));
    if (strcmp(buffer, PASSWORD) != 0) {
        std::cout << "❌ Authentication failed" << std::endl;
        SSL_write(ssl, "Authentication Failed", strlen("Authentication Failed"));
        SSL_shutdown(ssl);
        SSL_free(ssl);
        pthread_exit(NULL);
    } else {
        std::cout << "✅ Authentication successful" << std::endl;
        SSL_write(ssl, "Authentication Successful", strlen("Authentication Successful"));
    }

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = SSL_read(ssl, buffer, sizeof(buffer));
        if (bytes <= 0 || strcmp(buffer, "exit") == 0) break;

        std::cout << "📤 Executing command: " << buffer << std::endl;

        FILE* fp = popen(buffer, "r");
        if (!fp) {
            SSL_write(ssl, "Command failed", strlen("Command failed"));
            continue;
        }

        char output[4096] = {0};
        fread(output, 1, sizeof(output)-1, fp);
        pclose(fp);

        SSL_write(ssl, output, strlen(output));
    }

    std::cout << "📴 Client disconnected." << std::endl;

    SSL_shutdown(ssl);
    SSL_free(ssl);
    pthread_exit(NULL);
}
