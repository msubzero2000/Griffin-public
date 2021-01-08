#ifndef SOCKETSERVER_H
#define SOCKETSERVER_H

#include <stdint.h>
#include <thread>

namespace fly
{

#define BUFFSIZE 512

typedef struct payload_t {
    int32_t target_roll;
    int32_t target_pitch;
    int32_t game_state;
    int32_t lwing_angle;
    int32_t rwing_angle;
    int32_t body_height;
} payload;

class SocketServer
{
public:
    SocketServer();
    ~SocketServer();

    payload* getpayloads();
    void getLatestFromSocket();

private:

    char m_buff[BUFFSIZE]; 
    int m_ssock; 
    int m_csock;
    payload m_latestPayload;
    std::thread m_thread;
    bool m_running;
};

}

#endif // SOCKETSERVER_H
