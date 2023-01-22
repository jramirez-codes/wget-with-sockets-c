#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>


char * createSendBuffer(char* website, char* path, char * port) {
    // Length Of Data
    int dataLength = 1;
    dataLength += strlen("GET ");
    dataLength += strlen(path);
    dataLength += strlen(" HTTP/1.0\r\nHost: ");

    // If Local Host
    if(!strncmp(website, "localhost", 9)) {
        dataLength += strlen("127.0.0.1");
    }
    else {
        dataLength += strlen(website); 
    }
    // dataLength += strlen(website);

    dataLength += strlen(":");
    dataLength += strlen(port);
    dataLength += strlen("\r\n\r\n");

    char * sendBuff = malloc(sizeof(char) * dataLength);
    
    bzero(sendBuff, dataLength);
    sendBuff[dataLength - 1] = '\0';
    strcat(sendBuff, "GET ");
    strcat(sendBuff, path);
    strcat(sendBuff, " HTTP/1.0\r\nHost: ");

    // If localhost
    if(!strncmp(website, "localhost", 9)) {
        strcat(sendBuff, "127.0.0.1");
    }
    else {
        strcat(sendBuff, website);
    }

    strcat(sendBuff, ":");
    strcat(sendBuff, port);
    strcat(sendBuff, "\r\n\r\n");

    return sendBuff;
}

char * getFilename(char * path) {
    // Vars
    char * token = strtok(path, "/");
    char * filename;

    // Itterate through
    while(token != NULL) {
        // Get Filename buffer
        filename = malloc(sizeof(char) * (strlen(token) + 1));
        strcpy(filename, token);
        // Get next token
        token = strtok(NULL, "/");
        if(token == NULL) {
            return filename;
        }
        else {
            free(filename);
        }

    }

    // Path is path
    filename = malloc(sizeof(char) * strlen(path));
    strcpy(filename, path);
    return filename;
}

int parseHeader(char * filename, char * recvBuff) {
    // vars
    int length = 0;
    char * indexString = strtok(recvBuff, "\n");
    char * innerString;

    // Check inital html 200 response
    char * resCode = indexString + (sizeof(char) * 9);
    if(strncmp(resCode, "200", 3)) {
        fprintf(stdout, "%s\n", indexString);
        free(recvBuff);
        free(filename);
        exit(1);
    }

    // Get the length of the data
    indexString = strtok(NULL, "\n");
    while(indexString != NULL) {
        // Index through inner string
        innerString = strchr(indexString, ':');
        if(innerString != NULL) {
            // Check for content-length
            if(!strncmp(indexString, "Content-Length", strlen("Content-Length"))) {
                char * intString = innerString + (sizeof(char) * 2);
                return atoi(intString);
            }
        }
        indexString = strtok(NULL, "\n");
    }

    // Mandatory print [WHEN CANT FIND CONTENT LENGTH]
    fprintf(stdout, "Error: could not download the requested file (file length unknown)\n");
    exit(1);
}

void wget(char* argv[]) {
    // Socket Stuff
    struct addrinfo input, *res;
    int sockfd;
    
    // Set up input
    bzero(&input, sizeof(input));
    input.ai_family= AF_UNSPEC;
    input.ai_socktype = SOCK_STREAM;

    // Get Address
    if(!strncmp(argv[1], "localhost", 9)) {
        // Check to see if it is localhost
        getaddrinfo("127.0.0.1", argv[2], &input, &res);
    }
    else {
        getaddrinfo(argv[1], argv[2], &input, &res); 
    }
    // getaddrinfo(argv[1], argv[2], &input, &res);

    sockfd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    
    // Connect
    connect(sockfd,res->ai_addr,res->ai_addrlen);

    // Header for GET Request 
    char* sendBuff = createSendBuffer(argv[1], argv[3], argv[2]);

    // Send Get request
    send(sockfd, sendBuff, strlen(sendBuff),0);
    free(sendBuff);

    // Vars Data & Buffer
    int byteCount;
    char * recvBuff = malloc(sizeof(char)*2056);
    char * curr = malloc(sizeof(char) * 2);
    bzero(recvBuff, 2056);
    bzero(curr, 2);

    // Get the response header buffer
    int count = 0;
    char * endOfHeader = NULL;
    while(count < 2055 && endOfHeader == NULL) {
        byteCount = recv(sockfd, curr, sizeof(char), 0);
        if(byteCount == 1) {
            // Update Buffer
            curr[1] = '\0';
            recvBuff[count] = curr[0];
            // Look for the end of header
            endOfHeader = strstr(recvBuff, "\r\n\r\n");
            count++;
        }
    }
    recvBuff[count] = '\0';
    free(curr);
    // Parse header
    char * filename = getFilename(argv[3]);
    int contentLength = parseHeader(filename, recvBuff);
    FILE *file = fopen(filename, "wb");

    // Pull the rest of the file data
    while(contentLength > 0 && byteCount != 0) {
        if((byteCount = recv(sockfd,recvBuff,sizeof(recvBuff)-1,0)) != 0 && byteCount != -1) {
            recvBuff[byteCount] = '\0';
            // Only write data, not the response to file
            fwrite(recvBuff, 1, byteCount, file);
            contentLength -= byteCount;
        }
    }

    // Close & Free memory
    fclose(file);
    free(recvBuff);
    freeaddrinfo(res);
    free(filename);
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "usage: ./wget [host] [port number] [filepath]\n");
        exit(1);
    }

    // wget here
    wget(argv);

    return 0;
}

