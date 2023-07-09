#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#pragma comment(lib, "ws2_32.lib")

struct AcceptedSocket
{
    int acceptedSocketFD;
    struct sockaddr_in *address;
    int error;
    bool acceptedSuccessfully;
};

struct AcceptedSocket * acceptIncomingConnection(int serverSocketFD);
void acceptNewConnectionAndReceiveAndPrintItsData(int serverSocketFD);
void* receiveAndPrintIncomingData(void* socketFD);

void startAcceptingIncomingConnections(int serverSocketFD);

void receiveAndPrintIncomingDataOnSeparateThread(struct AcceptedSocket *pSocket);

void sendReceivedMessageToTheOtherClients(char *buffer,int socketFD);

struct AcceptedSocket acceptedSockets[10] ;
int acceptedSocketsCount = 0;


void startAcceptingIncomingConnections(int serverSocketFD) {

    while(true)
    {
        struct AcceptedSocket* clientSocket  = acceptIncomingConnection(serverSocketFD);
        acceptedSockets[acceptedSocketsCount++] = *clientSocket;
        receiveAndPrintIncomingDataOnSeparateThread(clientSocket);
    }
}



void receiveAndPrintIncomingDataOnSeparateThread(struct AcceptedSocket *pSocket) {


    pthread_t id;
    pthread_create(&id, NULL,  receiveAndPrintIncomingData, &pSocket->acceptedSocketFD);
    printf("\nNew thread created\n");
}

void * receiveAndPrintIncomingData(void* socketFd) {
    int socketFD =*((int*) socketFd);
    char buffer[1024];


    while (true)
    {
        ssize_t  amountReceived = recv(socketFD,buffer,1024,0);

        if(amountReceived>0)
        {
            buffer[amountReceived] = 0;
            printf("%s\n",buffer);

            sendReceivedMessageToTheOtherClients(buffer,socketFD);
        }

        if(amountReceived==0)
            break;
    }

    close(socketFD);
    free(socketFd);
}

void sendReceivedMessageToTheOtherClients(char *buffer,int socketFD) {

    for(int i = 0 ; i<acceptedSocketsCount ; i++)
        if(acceptedSockets[i].acceptedSocketFD !=socketFD)
        {
            send(acceptedSockets[i].acceptedSocketFD,buffer, strlen(buffer),0);
        }

}

struct AcceptedSocket * acceptIncomingConnection(int serverSocketFD) {
    struct sockaddr_in*  clientAddress ;
    int clientAddressSize = sizeof(struct sockaddr_in);
    int clientSocketFD = accept(serverSocketFD, (struct sockaddr *) clientAddress, &clientAddressSize);

    struct AcceptedSocket* acceptedSocket = malloc(sizeof (struct AcceptedSocket));
    acceptedSocket->address = clientAddress;
    acceptedSocket->acceptedSocketFD = clientSocketFD;
    acceptedSocket->acceptedSuccessfully = clientSocketFD>0;

    if(!acceptedSocket->acceptedSuccessfully)
        acceptedSocket->error = clientSocketFD;



    return acceptedSocket;
}


int createTCPIpv4Socket() {
    return socket(AF_INET, SOCK_STREAM, 0);
}
struct sockaddr_in *createIPv4Address(char *ip, int port) {
    struct sockaddr_in  *address = malloc(sizeof(struct sockaddr_in));
    address->sin_family = AF_INET;
    address->sin_port = htons(port);

    if(strlen(ip) ==0)
        address->sin_addr.s_addr = INADDR_ANY;
    else
        inet_pton(AF_INET,ip,&address->sin_addr.s_addr);

    return address;
}


int main() {

    int serverSocketFD = createTCPIpv4Socket();
    struct sockaddr_in *serverAddress = createIPv4Address("",2000);

    int result = bind(serverSocketFD, (const struct sockaddr *) serverAddress, sizeof(*serverAddress));
    if(result == 0)
        printf("socket was bound successfully\n");

    listen(serverSocketFD,10);

    startAcceptingIncomingConnections(serverSocketFD);

    shutdown(serverSocketFD,SHUT_RDWR);

    return 0;
}
