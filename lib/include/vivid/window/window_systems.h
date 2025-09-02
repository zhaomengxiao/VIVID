#pragma once

#include <SDL3/SDL.h>

#include "vivid/app/App.h"
#include "vivid/app/Plugin.h"

namespace VIVID::Window {

  class WindowPlugin : public Plugin {
  public:
    void build(App &app) override;
    std::string name() const override;
  };
}  // namespace VIVID::Window
