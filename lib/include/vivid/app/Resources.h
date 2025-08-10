#pragma once

#include <memory>
#include <unordered_map>
#include <typeindex>
#include <utility>

// 资源管理器
class Resources
{
private:
    std::unordered_map<std::type_index, std::unique_ptr<void, void (*)(void *)>> resources_;

public:
    template <typename T, typename... Args>
    T &insert(Args &&...args)
    {
        auto deleter = [](void *ptr)
        { delete static_cast<T *>(ptr); };
        auto resource = std::make_unique<T>(std::forward<Args>(args)...);
        T *ptr = resource.get();
        resources_.insert_or_assign(std::type_index(typeid(T)),
                                    std::unique_ptr<void, void (*)(void *)>(resource.release(), deleter));
        return *ptr;
    }

    template <typename T>
    T *get()
    {
        auto it = resources_.find(std::type_index(typeid(T)));
        if (it != resources_.end())
        {
            return static_cast<T *>(it->second.get());
        }
        return nullptr;
    }

    template <typename T>
    bool has() const
    {
        return resources_.find(std::type_index(typeid(T))) != resources_.end();
    }

    template <typename T>
    void remove()
    {
        resources_.erase(std::type_index(typeid(T)));
    }
};