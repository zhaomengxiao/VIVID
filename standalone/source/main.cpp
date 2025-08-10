#include "vivid/app/App.h"
#include "vivid/plugins/DefaultPlugin.h"
#include "editor/editor_plugin.h"
#include "vivid/rendering/render_plugin.h"

int main()
{
    auto &app = App::new_app().add_plugin<DefaultPlugin>().add_plugin<RenderPlugin>().add_plugin<EditorPlugin>();
    app.run();

    return 0;
}
