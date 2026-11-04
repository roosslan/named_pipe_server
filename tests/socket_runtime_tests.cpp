#include "socket_io.h"
#include <cstdio>
#include <future>
#include <stdexcept>
#include <thread>

using namespace std::chrono_literals;
void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
struct Socket {
    SOCKET value = INVALID_SOCKET;
    ~Socket() { if (value != INVALID_SOCKET) closesocket(value); }
};
struct Event {
    HANDLE value = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    ~Event() { if (value) CloseHandle(value); }
};

int main() {
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return 1;
    int result = 0;
    try {
        Socket listener{ socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) };
        require(listener.value != INVALID_SOCKET, "listener");
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        require(bind(listener.value, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0, "bind");
        int size = sizeof(address);
        require(getsockname(listener.value, reinterpret_cast<sockaddr*>(&address), &size) == 0, "getsockname");
        require(listen(listener.value, 1) == 0, "listen");
        Event stop;
        require(stop.value != nullptr, "event");
        Socket sender{ socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) };
        require(socket_io::connect(sender.value, reinterpret_cast<sockaddr*>(&address), sizeof(address), stop.value, 1s) == 0, "connect");
        Socket receiver{ accept(listener.value, nullptr, nullptr) };
        require(receiver.value != INVALID_SOCKET, "accept");
        require(socket_io::send_all(sender.value, "hello", stop.value, 1s) == 0, "normal send");
        char reply[5]{};
        require(recv(receiver.value, reply, sizeof(reply), MSG_WAITALL) == sizeof(reply) && std::string_view(reply, sizeof(reply)) == "hello", "message integrity");
        int small_buffer = 1024;
        setsockopt(sender.value, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&small_buffer), sizeof(small_buffer));
        setsockopt(receiver.value, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&small_buffer), sizeof(small_buffer));
        const std::string payload(32 * 1024 * 1024, 'x');
        auto start = std::chrono::steady_clock::now();
        require(socket_io::send_all(sender.value, payload, stop.value, 150ms) == WSAETIMEDOUT, "stalled reader must time out");
        require(std::chrono::steady_clock::now() - start < 1s, "send deadline exceeded");
        auto pending = std::async(std::launch::async, [&] {
            return socket_io::send_all(sender.value, payload, stop.value, 5s);
        });
        std::this_thread::sleep_for(50ms);
        SetEvent(stop.value);
        require(pending.wait_for(1s) == std::future_status::ready, "stop did not interrupt send");
        require(pending.get() == WSAEINTR, "send cancellation error");
        Socket cancelled{ socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) };
        require(socket_io::connect(cancelled.value, reinterpret_cast<sockaddr*>(&address), sizeof(address), stop.value, 1s) == WSAEINTR, "connect ignores stop");
        ResetEvent(stop.value);
        closesocket(listener.value);
        listener.value = INVALID_SOCKET;
        Socket refused{ socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) };
        require(socket_io::connect(refused.value, reinterpret_cast<sockaddr*>(&address), sizeof(address), stop.value, 2s) != 0, "closed port accepted connection");
        std::puts("PASS: TCP integrity, stalled-reader deadline, cancellation, refused connection");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        result = 1;
    }
    WSACleanup();
    return result;
}
