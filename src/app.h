#pragma once

#include "graphics.h"

// forward decls
struct GLFWwindow;
struct Chuck_VM;
struct Chuck_DL_Api;
typedef const Chuck_DL_Api* CK_DL_API;

struct CHUGL_App {
    GLFWwindow* window;
    GraphicsContext gctx;

    // Chuck Context
    Chuck_VM* ckvm;
    CK_DL_API ckapi;

    static void init(Chuck_VM* vm, CK_DL_API api);
    static void start();
    static void end();
};