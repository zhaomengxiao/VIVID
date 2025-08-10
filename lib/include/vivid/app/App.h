#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <type_traits>
#include <iostream>

#include "Plugin.h"
#include "Resources.h"
#include "Schedule.h"

// 应用程序主类
class App
{
private:
    entt::registry world_;
    Resources resources_;
    Schedule schedule_;
    std::vector<std::unique_ptr<Plugin>> plugins_;
    bool running_ = true;

public:
    App() = default;

    // 链式调用入口
    static App &new_app()
    {
        static App instance;
        return instance;
    }

    // 添加插件
    template <typename T, typename... Args>
    App &add_plugin(Args &&...args)
    {
        static_assert(std::is_base_of_v<Plugin, T>, "T must inherit from Plugin");
        auto plugin = std::make_unique<T>(std::forward<Args>(args)...);
        std::cout << "Adding plugin: " << plugin->name() << std::endl;
        plugin->build(*this);
        plugins_.push_back(std::move(plugin));
        return *this;
    }

    // 添加系统
    template <typename Fn>
    App &add_system(ScheduleLabel label, Fn &&fn)
    {
        schedule_.add_system(label, [fn = std::forward<Fn>(fn)](Resources &res, entt::registry &reg)
                             { fn(res, reg); });
        return *this;
    }

    // 添加无资源访问的系统，以保持兼容性
    App &add_system(ScheduleLabel label, std::function<void(entt::registry &)> fn)
    {
        schedule_.add_system(label, [fn = std::move(fn)](Resources &, entt::registry &reg)
                             { fn(reg); });
        return *this;
    }

    // 添加启动系统
    template <typename Fn>
    App &add_startup_system(Fn &&fn)
    {
        return add_system(ScheduleLabel::Startup, std::forward<Fn>(fn));
    }

    // 插入资源
    template <typename T, typename... Args>
    App &insert_resource(Args &&...args)
    {
        resources_.insert<T>(std::forward<Args>(args)...);
        return *this;
    }

    // 获取资源
    template <typename T>
    T *resource()
    {
        return resources_.get<T>();
    }

    // 获取世界
    entt::registry &world() { return world_; }
    const entt::registry &world() const { return world_; }

    // 获取资源管理器
    Resources &resources() { return resources_; }

    // 获取调度器
    Schedule &schedule() { return schedule_; }

    // 退出应用
    void exit() { running_ = false; }

    // 运行应用
    void run()
    {
        std::cout << "Starting application..." << std::endl;

        // 运行启动系统
        schedule_.run_schedule(ScheduleLabel::Startup, resources_, world_);

        // 主循环
        while (running_)
        {
            schedule_.run_schedule(ScheduleLabel::PreUpdate, resources_, world_);
            schedule_.run_schedule(ScheduleLabel::Update, resources_, world_);
            schedule_.run_schedule(ScheduleLabel::PostUpdate, resources_, world_);
            schedule_.run_schedule(ScheduleLabel::Render, resources_, world_);
            schedule_.run_schedule(ScheduleLabel::Cleanup, resources_, world_);
        }

        // --- Application Shutdown ---
        std::cout << "Application shutting down..." << std::endl;
        schedule_.run_schedule(ScheduleLabel::Shutdown, resources_, world_);

        std::cout << "Application finished." << std::endl;
    }
};