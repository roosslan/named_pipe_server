#include "stdafx.h"
#include "CNamedPipeServer.h"

using namespace w32;
using namespace std::chrono_literals;

void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

CHandle connect_client(const std::wstring& name) {
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    for (;;) {
        HANDLE handle = CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE,
            0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
        if (handle != INVALID_HANDLE_VALUE) {
            DWORD mode = PIPE_READMODE_MESSAGE;
            require(SetNamedPipeHandleState(handle, &mode, nullptr, nullptr), "client mode");
            return CHandle(handle);
        }
        require(std::chrono::steady_clock::now() < deadline, "client connect timed out");
        WaitNamedPipeW(name.c_str(), 50);
        std::this_thread::sleep_for(1ms);
    }
}

bool transfer(HANDLE handle, void* data, DWORD size, bool write, DWORD& count) {
    CManualResetEvent event;
    OVERLAPPED op{};
    op.hEvent = event;
    BOOL ok = write ? WriteFile(handle, data, size, &count, &op)
        : ReadFile(handle, data, size, &count, &op);
    if (!ok && GetLastError() != ERROR_IO_PENDING) return false;
    if (WaitForSingleObject(event, 5000) != WAIT_OBJECT_0) {
        CancelIoEx(handle, &op);
        GetOverlappedResult(handle, &op, &count, TRUE);
        throw std::runtime_error("client transfer timed out");
    }
    return GetOverlappedResult(handle, &op, &count, FALSE) != FALSE;
}

void echo(void*, CHandle&, CIOBuffer& input, CIOBuffer& output) {
    if (input.Offset() > output.Size()) output.AddSize(input.Offset() - output.Size());
    std::memcpy(output.Ptr(), input.Ptr(), input.Offset());
    output.SetOffset(input.Offset());
}

void exchange_message(HANDLE client, const std::string& value) {
    DWORD count = 0;
    require(transfer(client, const_cast<char*>(value.data()), static_cast<DWORD>(value.size()), true, count), "write");
    require(count == value.size(), "short write");
    std::string reply(value.size(), '\0');
    require(transfer(client, reply.data(), static_cast<DWORD>(reply.size()), false, count), "read");
    require(count == value.size() && reply == value, "corrupted message");
}

int main() {
    try {
        const auto base = L"\\\\.\\pipe\\bghelper_test_" + std::to_wstring(GetCurrentProcessId());
        {
            CNamedPipeServer server(base, echo, nullptr, 256, 256, 10, 4);
            server.StartServing();
            std::vector<std::future<void>> clients;
            for (int i = 0; i < 6; ++i) {
                clients.push_back(std::async(std::launch::async, [&, i] {
                    for (int reconnect = 0; reconnect < 30; ++reconnect) {
                        auto client = connect_client(base);
                        for (int n = 0; n < 5; ++n)
                            exchange_message(client, std::to_string(i) + ":" + std::to_string(n) + std::string(n % 2 ? 8192 : 32, 'x'));
                    }
                }));
            }
            for (auto& client : clients) client.get();
            auto idle = connect_client(base);
            auto large = connect_client(base);
            exchange_message(large, std::string(1024 * 1024, 'L'));
            server.Shutdown();
            require(server.WaitUntilFinished(3000), "idle shutdown timed out");
        }
        {
            std::atomic<bool> entered = false;
            auto handler = [](void* context, CHandle&, CIOBuffer&, CIOBuffer& output) {
                output.AddSize(1024 * 1024 - output.Size());
                output.SetOffset(1024 * 1024);
                *static_cast<std::atomic<bool>*>(context) = true;
            };
            CNamedPipeServer server(base + L"_blocked", handler, &entered, 256, 256, 10, 4);
            server.StartServing();
            auto client = connect_client(base + L"_blocked");
            char request = 'x'; DWORD count = 0;
            require(transfer(client, &request, 1, true, count), "blocked client write");
            const auto deadline = std::chrono::steady_clock::now() + 3s;
            while (!entered && std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(1ms);
            require(entered, "handler did not run");
            server.Shutdown();
            require(server.WaitUntilFinished(3000), "blocked reply shutdown timed out");
        }
        {
            CNamedPipeServer server(base + L"_limit", echo, nullptr, 256, 256, 10, 4);
            server.StartServing();
            auto client = connect_client(base + L"_limit");
            std::string oversized(1024 * 1024 + 1, 'x'); DWORD count = 0;
            const bool written = transfer(client, oversized.data(), static_cast<DWORD>(oversized.size()), true, count);
            if (written) {
                char reply;
                require(!transfer(client, &reply, 1, false, count), "oversized message accepted");
            }
            auto healthy = connect_client(base + L"_limit");
            exchange_message(healthy, "still serving");
            server.Shutdown();
            require(server.WaitUntilFinished(3000), "limit shutdown timed out");
        }
        std::puts("PASS: concurrent reconnects, message integrity, 1 MiB boundary, idle/blocked shutdown, oversize rejection");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s (Win32 %lu)\n", error.what(), GetLastError());
        return 1;
    }
}
