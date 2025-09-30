#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#define OUTFILE "/var/tmp/aesdsocketdata"
uint8_t running;

void handle_signal(int sig)  {
    if (sig == SIGINT || sig == SIGTERM) {
        running = 0;
        syslog(LOG_INFO, "Caught signal, exiting");
        remove(OUTFILE);
        printf("exit\r\n");
        exit(-1);
    }
}

int main(int argc, char **args) {
    int daemon_mode = 0;
    running = 1;
    //check daemon mode
    if (argc > 1 && strcmp(args[1], "-d") == 0) {
        daemon_mode = 1;
    }

    openlog("aesdsocket", 0, LOG_USER);

    //Register the signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    struct addrinfo hints;
    struct addrinfo *serverinfo;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, "9000", &hints, &serverinfo) != 0) {
        perror("getaddrinfo");
        exit(-1);
    }

    int sockfd = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol);
    if (sockfd == -1) {
        perror("socket");
        exit(-1);
    }

    // Allow reuse of address
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt");
        exit(-1);
    }

    if (bind(sockfd, serverinfo->ai_addr, serverinfo->ai_addrlen) != 0) {
        perror("bind");
        exit(-1);
    }

    if (daemon_mode){
        pid_t pid = fork();
        if (pid < 0){
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid > 0){
            exit(EXIT_SUCCESS);
        }

        if (setsid() < 0){
            perror("setsid");
            exit(EXIT_FAILURE);
        }

        if (chdir("/") < 0){
            perror("chdir");
            exit(EXIT_FAILURE);
        }
        freopen("/dev/null", "r", stdin);
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
    }

    if (listen(sockfd, 10) != 0) {
        perror("listen");
        exit(-1);
    }

    while(running) {
        struct sockaddr clientaddr;
        socklen_t clientaddr_len;
        char ipaddr[INET_ADDRSTRLEN];
        char buff[1024];
        
        int newsockfd = accept(sockfd, &clientaddr, &clientaddr_len);
        if (newsockfd == -1) {
            perror("accept");
            exit(-1);
        }
        inet_ntop(AF_INET, &((struct sockaddr_in *)&clientaddr)->sin_addr, ipaddr, sizeof(ipaddr));
        syslog(LOG_INFO, "Accepted connection from %s\n", ipaddr);
        printf("Accepted connection from %s\n", ipaddr);

        FILE *fd = fopen(OUTFILE, "a+");
        if (!fd){
            perror("fopen");
            close(newsockfd);
            exit(-1);
        }

        bool packet_complete = false;
        while(!packet_complete && running) {
            int bytes = recv(newsockfd, buff, sizeof(buff) - 1, 0);
            if(bytes == -1){
                perror("receive");
                break;
            }else if (bytes == 0){
                break;
            }

            buff[bytes] = '\0';
            char *newline = strchr(buff, '\n');
            if (newline != NULL){
                size_t to_write = (newline - buff) + 1;
                if (fwrite(buff, 1, to_write, fd) < to_write){
                    perror("fwrite");
                    break;
                }
                fflush(fd);
                break;
            } else {
                if (fwrite(buff, 1, bytes, fd) < (size_t)bytes){
                    perror("fwrite");
                    break;
                }
            }
        }
        fclose(fd);

        // return file content to sender
        FILE *fd2 = fopen(OUTFILE, "r");
        if (!fd2){
            perror("fopen");
            close(newsockfd);
            exit(-1);
        }
        char sendbuf[1024];
        size_t bytes_read;
        ssize_t bytes_sent;

        while((bytes_read = fread(sendbuf, 1, 1024, fd2)) > 0){
            size_t total_sent = 0;
            while (total_sent < bytes_read){
                bytes_sent = send(newsockfd, sendbuf + total_sent, bytes_read - total_sent, 0);
                if (bytes_sent == -1){
                    perror("send");
                    break;
                }
                total_sent += bytes_sent;
            }
        }
        fclose(fd2);
        close(newsockfd);
        syslog(LOG_INFO, "Accepted connection from %s\n", ipaddr);
        printf("Closed connection from %s\n", ipaddr);
    }
    close(sockfd);
    closelog();
    freeaddrinfo(serverinfo);
    remove(OUTFILE);
    return 0;
}