#include "SocketServer.h"
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(1)

namespace fly {

#pragma pack()

int createSocket(int port)
{
    int sock, err;
    struct sockaddr_in server;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("ERROR: Socket creation failed\n");
        exit(1);
    }
    printf("Socket created\n");

    bzero((char *) &server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        printf("ERROR: Bind failed\n");
        exit(1);
    }
    printf("Bind done\n");

    listen(sock , 3);

    return sock;
}

void closeSocket(int sock)
{
    close(sock);
    return;
}

void sendMsg(int sock, void* msg, uint32_t msgsize)
{
    if (write(sock, msg, msgsize) < 0)
    {
        printf("Can't send message.\n");
        closeSocket(sock);
        exit(1);
    }
    printf("Message sent (%d bytes).\n", msgsize);
    return;
}

SocketServer::SocketServer():
    m_running(false)
{
    m_latestPayload.target_roll = 0;
    m_latestPayload.target_pitch = 0;
    m_latestPayload.lwing_angle = 0;
    m_latestPayload.game_state = 0;
    m_latestPayload.body_height = 0;
    m_latestPayload.rwing_angle = 0;
    
    int PORT = 2300;
    int nread;
    struct sockaddr_in client;
    unsigned int clilen = sizeof(client);

    m_ssock = createSocket(PORT);
    printf("Server listening on port %d\n", PORT);
    
    m_csock = accept(m_ssock, (struct sockaddr *)&client, &clilen);
    if (m_csock < 0)
    {
        printf("Error: accept() failed\n");
    } else {        
        printf("Accepted connection from %s\n", inet_ntoa(client.sin_addr));
        bzero(m_buff, BUFFSIZE);
    }
}

void SocketServer::getLatestFromSocket()
{
    while (m_running) {
        int nread = read(m_csock, m_buff, BUFFSIZE);
        if (nread > 0)
        {
            //printf("\nReceived %d bytes\n", nread);
            payload *p = (payload*) m_buff;
            m_latestPayload = *p;            
        }
    }
}

SocketServer::~SocketServer()
{
    if (m_csock >= 0) {
        closeSocket(m_csock);
    }
    if (m_ssock >= 0) {
        closeSocket(m_ssock);
    }
    m_running = false;
    m_thread.join();
}

void test(std::string msg) {
}

payload* SocketServer::getpayloads()
{
    if (m_running == false) {
        m_thread = std::thread(&SocketServer::getLatestFromSocket, this);
        m_running = true;
    }

    printf("Received contents: troll=%d, tpitch=%d gstate=%d lwangle=%d rwangle=%d bheight=%d\n",
            m_latestPayload.target_roll, m_latestPayload.target_pitch, m_latestPayload.game_state, m_latestPayload.lwing_angle,
            m_latestPayload.rwing_angle, m_latestPayload.body_height);
    
    return &m_latestPayload;
}

}
