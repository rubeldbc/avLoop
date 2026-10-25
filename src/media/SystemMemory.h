#pragma once

#include <QString>

#include <cstdint>

namespace avloop::media {

// What the status bar and --mem-log show (spec §3.7.7, §3.7.8): the process's Private Bytes, the GPU memory it uses
// (DXGI), free system RAM, and the GPU's name (spec §3.6's status line).
struct MemorySnapshot {
    std::int64_t privateBytes = 0;
    std::int64_t vramUsed = 0;
    std::int64_t vramBudget = 0;
    std::int64_t freePhysical = 0;
    std::int64_t totalPhysical = 0;
};

class SystemMemory {
public:
    static MemorySnapshot read();
    static QString adapterName();
    static std::int64_t adapterMemory(); // dedicated video memory of the first adapter
};

} // namespace avloop::media
