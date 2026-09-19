#include "AdvControl.h"

namespace Adv
{
    void AdvControl::BeginFrame()
    {
        tasks.clear();
    }

    void AdvControl::AddTask(
        TaskType type,
        uint32_t count,
        uint32_t inputBuffer,
        uint32_t outputBuffer)
    {
        tasks.push_back(Task{
            type,
            count,
            inputBuffer,
            outputBuffer
        });
    }

    void AdvControl::Execute()
    {
        // Phase 0.1: this is the CPU-side task description layer.
        // Future phases will translate tasks into GPU work:
        // graphics / compute / copy queues, GPU workers, fences and indirect dispatch.
    }
}
