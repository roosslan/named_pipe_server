#pragma once
#include <winsock2.h>
#include <algorithm>
#include <chrono>
#include <string_view>

namespace socket_io {
    // The caller owns the socket and serializes access. Successful connections
    // remain nonblocking so send_all can enforce its total operation deadline.
    inline int connect(SOCKET socket, const sockaddr* address, int address_size,
        HANDLE stop, std::chrono::milliseconds timeout) {
        if (WaitForSingleObject(stop, 0) == WAIT_OBJECT_0) return WSAEINTR;
        u_long nonblocking = 1;
        if (ioctlsocket(socket, FIONBIO, &nonblocking) == SOCKET_ERROR) return WSAGetLastError();
        if (::connect(socket, address, address_size) == 0) return 0;
        const int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK) return error;
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        for (;;) {
            if (WaitForSingleObject(stop, 0) == WAIT_OBJECT_0) return WSAEINTR;
            const auto remaining = deadline - std::chrono::steady_clock::now();
            if (remaining <= std::chrono::steady_clock::duration::zero()) return WSAETIMEDOUT;
            fd_set writable, errors;
            FD_ZERO(&writable); FD_SET(socket, &writable);
            FD_ZERO(&errors); FD_SET(socket, &errors);
            const auto wait_us = (std::min)(std::chrono::duration_cast<std::chrono::microseconds>(remaining).count(), 50000LL);
            timeval wait{ 0, static_cast<long>(wait_us) };
            const int ready = select(0, nullptr, &writable, &errors, &wait);
            if (ready == SOCKET_ERROR) return WSAGetLastError();
            if (ready == 0) continue;
            int result = 0;
            int size = sizeof(result);
            if (getsockopt(socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&result), &size) == SOCKET_ERROR)
                return WSAGetLastError();
            return result;
        }
    }

    inline int send_all(SOCKET socket, std::string_view data, HANDLE stop,
        std::chrono::milliseconds timeout) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (!data.empty()) {
            if (WaitForSingleObject(stop, 0) == WAIT_OBJECT_0) return WSAEINTR;
            const auto remaining = deadline - std::chrono::steady_clock::now();
            if (remaining <= std::chrono::steady_clock::duration::zero()) return WSAETIMEDOUT;
            const int sent = send(socket, data.data(), static_cast<int>((std::min)(data.size(), size_t{ 64 * 1024 })), 0);
            if (sent == SOCKET_ERROR) {
                const int error = WSAGetLastError();
                if (error != WSAEWOULDBLOCK) return error;
                const auto wait_ms = (std::min)(std::chrono::duration_cast<std::chrono::milliseconds>(remaining).count(), 50LL);
                WaitForSingleObject(stop, static_cast<DWORD>(wait_ms));
                continue;
            }
            if (sent == 0) return WSAECONNRESET;
            data.remove_prefix(static_cast<size_t>(sent));
        }
        return 0;
    }
}
