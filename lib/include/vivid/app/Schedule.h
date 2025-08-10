#pragma once

#include <entt/entt.hpp>
#include <functional>
#include <vector>
#include <unordered_map>
#include <utility>
#include "Resources.h"

// 系统函数类型
using SystemFn = std::function<void(Resources &, entt::registry &)>;

// 调度阶段枚举
enum class ScheduleLabel
{
    Startup,
    PreUpdate,
    Update,
    PostUpdate,
    Render,
    Cleanup,
    Shutdown
};

// 系统调度器
class Schedule
{
private:
    std::unordered_map<ScheduleLabel, std::vector<SystemFn>> systems_;

public:
    void add_system(ScheduleLabel label, SystemFn system)
    {
        systems_[label].push_back(std::move(system));
    }

    void run_schedule(ScheduleLabel label, Resources &resources, entt::registry &registry)
    {
        auto it = systems_.find(label);
        if (it != systems_.end())
        {
            if (label == ScheduleLabel::Shutdown)
            {
                // Execute shutdown systems in reverse order of registration (LIFO)
                // to ensure dependencies are handled correctly.
                for (auto rit = it->second.rbegin(); rit != it->second.rend(); ++rit)
                {
                    (*rit)(resources, registry);
                }
            }
            else
            {
                for (auto &system : it->second)
                {
                    system(resources, registry);
                }
            }
        }
    }

    void clear_schedule(ScheduleLabel label)
    {
        auto it = systems_.find(label);
        if (it != systems_.end())
        {
            it->second.clear();
        }
    }
};