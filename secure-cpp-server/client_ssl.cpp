#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 8080

int main() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    const SSL_METHOD* method = TLS_client_method();
    SSL_CTX* ctx = SSL_CTX_new(method);

    if (!ctx) {
        std::cerr << "Unable to create SSL context" << std::endl;
        ERR_print_errors_fp(stderr);
        return -1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed\n";
        return -1;
    }

    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);

    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        close(sock);
        return -1;
    }

    std::string input;
    char buffer[4096] = {0};

    std::cout << "Enter Password: ";
    std::getline(std::cin, input);
    SSL_write(ssl, input.c_str(), input.length());

    SSL_read(ssl, buffer, sizeof(buffer));
    std::cout << buffer << std::endl;

    if (std::string(buffer).find("Failed") != std::string::npos) {
        SSL_free(ssl);
        close(sock);
        return 0;
    }

    while (true) {
        std::cout << "Enter Command (or 'exit' to quit): ";
        std::getline(std::cin, input);
        SSL_write(ssl, input.c_str(), input.length());

        if (input == "exit") break;

        memset(buffer, 0, sizeof(buffer));
        SSL_read(ssl, buffer, sizeof(buffer));
        std::cout << "Output:\n" << buffer << std::endl;
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sock);
    SSL_CTX_free(ctx);
    EVP_cleanup();
    return 0;
}
