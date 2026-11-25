#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

struct sockaddr_in addr;

void send_404(int client_fd){
    const char *not_found = 
    "HTTP/1.1 404 Not Found \r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length:9\r\n"
    "Connection: close\r\n"
    "\r\n"
    "Not Found";
    write(client_fd, not_found, strlen(not_found));
}

void serve_file(int client_fd, const char *filename, const char *content_type){
    int file_fd = open(filename, O_RDONLY);
    if(file_fd < 0){
        perror("open failed");
        send_404(client_fd);
        return;
    }

    struct stat st;
    if(fstat(file_fd, &st) < 0){
        perror("fstat failed");
        close(file_fd);
        send_404(client_fd);
        return;
    }

    char header[256];
    int header_len = snprintf(header, sizeof(header), 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %ld\r\n"
    "Connection: close\r\n"
    "\r\n", content_type, (long)st.st_size);

    write(client_fd, header, header_len);
    sendfile(client_fd, file_fd, NULL, st.st_size);
    close(file_fd);
}

int main(){
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);
    if(bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("bind failed");
        close(server_fd);
        return 1;
    }
    listen(server_fd, 10);

    while(true){
        int client_fd = accept(server_fd, NULL, NULL);
        char buffer[4096];
        int bytes_read = read(client_fd, buffer, sizeof(buffer)-1);
        buffer[bytes_read] ='\0';
        printf("%s\n", buffer);
        if (strncmp(buffer, "GET / ", 6) == 0)
            serve_file(client_fd, "index.html", "text/html; charset=utf-8");
        else if(strncmp(buffer, "GET /cat.jpg ", 13) == 0)
            serve_file(client_fd, "cat.jpg", "image/jpeg");
        else
            send_404(client_fd);
        
        close(client_fd);

    }
}