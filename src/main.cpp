// standalone main for testing the renderer only
// does NOT link with chuck or chugin.h in any way

// TODO: rename this to "test_all"

#include "all.cpp"

// #include "tests/gltf.cpp"
// #include "tests/obj.cpp"

// void Test_Obj(TestCallbacks* callbacks);
void Test_Gltf(TestCallbacks* callbacks);

int main(int, char**)
{
    log_trace("main");
    App app        = {};
    app.standalone = true;

    // initialize component manager (TODO do this in App instead?)
    Component_Init();

    // load test entry points
    // Test_Obj(&app.callbacks);
    Test_Gltf(&app.callbacks);

    App::init(&app, NULL, NULL);
    App::start(&app);
    App::end(&app);

    Component_Free();
    return EXIT_SUCCESS;
}