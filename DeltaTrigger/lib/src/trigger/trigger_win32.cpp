/*
 * Copyright 2026 Jakub Bączyk
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN

#include <trigger/trigger.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

using SignalSocket = trigger::SignalSocket;

static bool initSocket(SignalSocket& sock)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        sock = trigger::INVALID_SIGNAL_SOCKET;
        return false;
    }

    sock = static_cast<SignalSocket>(
        socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)
    );

    if (sock == INVALID_SOCKET)
    {
        sock = trigger::INVALID_SIGNAL_SOCKET;
        return false;
    }

    return true;
}

SignalSocket trigger::StartServer(int port)
{
    SignalSocket sock;
    if (!initSocket(sock))
        return INVALID_SIGNAL_SOCKET;

    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.S_un.S_addr = htonl(INADDR_LOOPBACK);

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        Close(sock);
        return INVALID_SIGNAL_SOCKET;
    }

    return sock;
}

bool trigger::CheckForSignal(SignalSocket serverSock)
{
    if (serverSock == INVALID_SIGNAL_SOCKET)
        return false;

    char buf;
    bool triggered = false;

    while (recvfrom(serverSock, &buf, 1, 0, nullptr, nullptr) > 0)
        triggered = true;

    return triggered;
}

bool trigger::Send(int port)
{
    SignalSocket sock;
    if (!initSocket(sock))
        return false;

    sockaddr_in dest = {};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    dest.sin_addr.S_un.S_addr = htonl(INADDR_LOOPBACK);

    char signal = 67;
    int result = sendto(sock, &signal, 1, 0, (sockaddr*)&dest, sizeof(dest));

    Close(sock);
    return result > 0;
}

void trigger::Close(SignalSocket& sock)
{
    if (sock != INVALID_SIGNAL_SOCKET)
    {
        closesocket(sock);
        sock = INVALID_SIGNAL_SOCKET;
    }
}

#endif
