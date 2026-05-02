#include<stdio.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netdb.h>

#define PORT 8080
#define BUFFER_SIZE 4096 

void handle_client(int client_fd){

    char request[BUFFER_SIZE];
    memset(request, 0, BUFFER_SIZE);

    int bytes = recv(client_fd, request, BUFFER_SIZE - 1, 0);
    if (bytes <= 0) {
        close(client_fd);
        return;
    }
	
    printf("Received request:\n%s\n", request);
    char method[8], path[256], version[16];
    sscanf(request, "%7s %255s %15s", method, path, version);

    // Only allow GET
    if(strcmp(method, "GET") != 0){
        char *response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
        send(client_fd, response, strlen(response), 0);
        close(client_fd);
        return;
    }

    // Prevent path traversal
    if (strstr(path, "..")) {
        char *response = "HTTP/1.1 400 Bad Request\r\n\r\n";
        send(client_fd, response, strlen(response), 0);
        close(client_fd);
        return;
    }

    // Default file
    if(strcmp(path, "/") == 0){
        strcpy(path, "/index.html");
    }

    char file_path[512];
    snprintf(file_path, sizeof(file_path), ".%s", path);

    FILE *file = fopen(file_path, "rb");

    if(!file){
        char *response =
            "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found";
        send(client_fd, response, strlen(response), 0);
        close(client_fd);
        return;
    }

    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    rewind(file);

    char header[512];
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n",
        file_size);

    send(client_fd, header, strlen(header), 0);

    char file_buffer[1024];
    int bytes_read;

    while((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), file)) > 0){
        send(client_fd, file_buffer, bytes_read, 0);
    }

    fclose(file);
    close(client_fd);
}


int main(){
    
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // ---- Create socket ----
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // ---- Configure address ----
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // ---- Bind ----
    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // ---- Listen ----
    listen(server_fd, 10);

    printf("Server running on http://localhost:%d\n", PORT);

    // ---- Main loop ----
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        handle_client(client_fd);
    }

    close(server_fd);
    return 0;
}
