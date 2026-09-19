#pragma once

#include <cstdint>
#include <vector>

namespace Adv
{
    enum class TaskType : uint32_t
    {
        Clear,
        Geometry,
        Culling,
        LOD,
        Shadows,
        Lighting,
        Particles,
        PostProcess
    };

    struct Task
    {
        TaskType type = TaskType::Clear;
        uint32_t count = 0;
        uint32_t inputBuffer = 0;
        uint32_t outputBuffer = 0;
    };

    class AdvControl
    {
    public:
        void BeginFrame();

        void AddTask(
            TaskType type,
            uint32_t count = 0,
            uint32_t inputBuffer = 0,
            uint32_t outputBuffer = 0
        );

        void Execute();

        const std::vector<Task>& GetTasks() const { return tasks; }

    private:
        std::vector<Task> tasks;
    };
}
