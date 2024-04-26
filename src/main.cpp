// standalone main for testing the renderer only
// does NOT link with chuck or chugin.h in any way

// TODO: convert into unity build

#include "all.cpp"

int main(int, char**)
{
    log_trace("main");
    App app        = {};
    app.standalone = true;
    App::init(&app, NULL, NULL);
    App::start(&app);
    App::end(&app);
    return EXIT_SUCCESS;
}