#pragma once

#include "graphics.h"

// forward decls
struct GLFWwindow;
struct Chuck_VM;
struct Chuck_DL_Api;
typedef const Chuck_DL_Api* CK_DL_API;

struct AppOptions {
    bool standalone; // no chuck. renderer only
};

void App_Init(Chuck_VM* vm, CK_DL_API api, AppOptions* options);
void App_Start();
void App_End();