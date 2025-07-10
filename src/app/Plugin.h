#pragma once

#include <string>

// 前向声明
class App;

// 插件接口
class Plugin
{
public:
    virtual ~Plugin() = default;
    virtual void build(App &app) = 0;
    virtual std::string name() const = 0;
};