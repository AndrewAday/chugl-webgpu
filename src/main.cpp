// standalone main for testing the renderer only
// does NOT link with chuck or chugin.h in any way

// TODO: convert into unity build

#include <cstdlib>

#include "app.h"
#include "core/log.h"
#include "core/macros.h"

int main(int, char**)
{
    log_trace("main");
    AppOptions options = {};
    options.standalone = true;
    App_Init(NULL, NULL, &options);
    App_Start();
    App_End();
    return EXIT_SUCCESS;
}