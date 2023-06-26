#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

uint32_t getFileSizeInNetworkBytesOrder(int file){ //used GPT for guidens on that function
    off_t size;
    uint32_t result;
    size = lseek(file, 0, SEEK_END); // Get the size of the file
    lseek(file, 0, SEEK_SET); // Move file pointer back to beginning of file
    result = htonl(size);
    return result;
}

void sentFileContentToServer(int socketDescriptor, int file){
    int sizeOfReadChars =0;
    int sizeOfSentChars = 0;
    char buff[1000000];
    while(1){
        sizeOfReadChars = read(file, buff, sizeof(buff) - 1);
        if (sizeOfReadChars < 0){
            perror("Failed to read from file");
            exit(1); 
        }
        if (sizeOfReadChars > 0){
            sizeOfSentChars = write(socketDescriptor, buff, sizeOfReadChars);
            if (sizeOfSentChars < sizeOfReadChars){
                perror("Failed to send data from file to server");
                exit(1); 
            }            
        }
        if(sizeOfReadChars == 0){
            break;
        }
    }
}

int main(int argc, char *argv[]){
    uint32_t printableCharNum = 0;
    int socketDescriptor =-1;
    int file;
    int sereverPort;
    int printableCharNumInINT;
    char* serverIPAddress;
    char* outputBuffer;
    char* inputBuffer;
    struct in_addr IP;
    struct sockaddr_in server_addr;
    uint32_t fileSize;
    int result;
    //command line arguments number validation 
    if (argc != 4){
        perror("Wrong number of command line arguments");
        exit(1);
    }
    serverIPAddress = argv[1];
    sereverPort = atoi(argv[2]);
    //opening the file for reading
    file = open(argv[3],O_RDWR);
    if (file < 0){
        perror("Failed on open file");
        exit(1);
    }
    // creating a TCP connection to the specified server port on the specified server IP
         // creat a socket
    socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socketDescriptor < 0){
        perror("failed to creat socket");
        exit(1);
    }
         // set up server
    if(inet_pton(AF_INET, serverIPAddress, &IP) != 1){
        perror("Failed to setup server");
        exit(1);
	}
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(sereverPort);
    server_addr.sin_addr.s_addr = inet_addr(serverIPAddress);
         // connect socket to the server
    if (connect(socketDescriptor, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0){
        perror("failed to connect socket to server");
        exit(1);
    }
    //make the output to send the server
    fileSize = getFileSizeInNetworkBytesOrder(file);
    outputBuffer = (char*)&fileSize;
    //sent N to server
    if(write(socketDescriptor, outputBuffer, 4)<0){
        perror("failed to csend N to server");
        exit(1);
    }
    //send file's content to server
    sentFileContentToServer(socketDescriptor, file);
    //getting the number of printable characters from server
    inputBuffer = (char*)&printableCharNum;
    printableCharNumInINT = read(socketDescriptor, inputBuffer, 4);
    if (printableCharNumInINT < 0){
        perror("failed to get the N number from server");
        exit(1);
    }
    //printing the number of printable characters from server
    result = ntohl(printableCharNum);
    printf(" # of printable characters: %u\n", result);
    close(socketDescriptor);
    exit(0);
}