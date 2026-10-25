#include "media/SystemMemory.h"

#include <windows.h>

#include <dxgi1_4.h>
#include <psapi.h>
#include <wrl/client.h>

#include <mutex>

namespace avloop::media {

namespace {

using Microsoft::WRL::ComPtr;

// The first hardware adapter, kept for the process (the one D3D11VA and OpenGL use by default).
struct Adapter {
    ComPtr<IDXGIAdapter3> adapter;
    QString name;
    std::int64_t dedicated = 0;
};

const Adapter& adapter() {
    static Adapter a;
    static std::once_flag once;
    std::call_once(once, [] {
        ComPtr<IDXGIFactory1> factory;
        if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return;
        ComPtr<IDXGIAdapter1> first;
        for (UINT i = 0; factory->EnumAdapters1(i, &first) != DXGI_ERROR_NOT_FOUND; ++i) {
            DXGI_ADAPTER_DESC1 desc{};
            first->GetDesc1(&desc);
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
            a.name = QString::fromWCharArray(desc.Description);
            a.dedicated = std::int64_t(desc.DedicatedVideoMemory);
            first.As(&a.adapter);
            break;
        }
    });
    return a;
}

} // namespace

MemorySnapshot SystemMemory::read() {
    MemorySnapshot s;
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof pmc))
        s.privateBytes = std::int64_t(pmc.PrivateUsage);
    MEMORYSTATUSEX ms{};
    ms.dwLength = sizeof ms;
    if (GlobalMemoryStatusEx(&ms)) {
        s.freePhysical = std::int64_t(ms.ullAvailPhys);
        s.totalPhysical = std::int64_t(ms.ullTotalPhys);
    }
    if (const auto& a = adapter(); a.adapter) {
        DXGI_QUERY_VIDEO_MEMORY_INFO info{};
        if (SUCCEEDED(a.adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info))) {
            s.vramUsed = std::int64_t(info.CurrentUsage);
            s.vramBudget = std::int64_t(info.Budget);
        }
    }
    return s;
}

QString SystemMemory::adapterName() {
    return adapter().name;
}

std::int64_t SystemMemory::adapterMemory() {
    return adapter().dedicated;
}

} // namespace avloop::media
