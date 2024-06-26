#include <chuck/chugin.h>

#include "ulib_helper.h"

/* API

// window frame size
int window_frame_left, window_frame_top, window_frame_right,
window_frame_bottom;

float window_opacity;

glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);


----------------

window.title();

class GWindow
{
    title()
}

// window closing

window.shouldCloseEvent => now;

-----
window attribs

GLFW_HOVERED indicates whether the cursor is currently directly over the content
area of the window, with no other windows between. See Cursor enter/leave events
for details.

GLFW_VISIBLE indicates whether the specified window is visible. See Window
visibility for details.

GLFW_RESIZABLE indicates whether the specified window is resizable by the user.
This can be set before creation with the GLFW_RESIZABLE window hint or after
with glfwSetWindowAttrib.

GLFW_DECORATED indicates whether the specified window has decorations such as a
border, a close widget, etc. This can be set before creation with the
GLFW_DECORATED window hint or after with glfwSetWindowAttrib.

GLFW_AUTO_ICONIFY indicates whether the specified full screen window is
iconified on focus loss, a close widget, etc. This can be set before creation
with the GLFW_AUTO_ICONIFY window hint or after with glfwSetWindowAttrib.

GLFW_FLOATING indicates whether the specified window is floating, also called
topmost or always-on-top. This can be set before creation with the GLFW_FLOATING
window hint or after with glfwSetWindowAttrib.

GLFW_TRANSPARENT_FRAMEBUFFER indicates whether the specified window has a
transparent framebuffer, i.e. the window contents is composited with the
background using the window framebuffer alpha channel. See Window transparency
for details.

GLFW_FOCUS_ON_SHOW specifies whether the window will be given input focus when
glfwShowWindow is called. This can be set before creation with the
GLFW_FOCUS_ON_SHOW window hint or after with glfwSetWindowAttrib.

----
Window size limits
- constrain min/max size
- constrain aspect ratio



*/

// monitor (not implemented)
CK_DLL_SFUN(gwindow_monitor_info);
static t_CKINT monitor_info_width_offset            = 0;
static t_CKINT monitor_info_height_offset           = 0;
static t_CKINT monitor_info_refresh_rate_offset     = 0;
static t_CKINT monitor_info_virtual_pos_offset      = 0;
static t_CKINT monitor_info_physical_size_mm_offset = 0;
static t_CKINT monitor_info_content_scale_offset    = 0;
static t_CKINT monitor_info_work_area_offset        = 0;
static t_CKINT monitor_info_name_offset             = 0;

// callbacks
CK_DLL_SFUN(gwindow_close_event);
CK_DLL_SFUN(gwindow_window_resize_event);
CK_DLL_SFUN(gwindow_window_content_scale_event);

// closing
CK_DLL_SFUN(gwindow_set_closeable);
CK_DLL_SFUN(gwindow_get_closeable);
CK_DLL_SFUN(gwindow_close);

// window size
CK_DLL_SFUN(gwindow_fullscreen);
CK_DLL_SFUN(gwindow_fullscreen_width_height);
CK_DLL_SFUN(gwindow_windowed);
CK_DLL_SFUN(gwindow_maximize);

CK_DLL_SFUN(gwindow_get_window_size);
CK_DLL_SFUN(gwindow_get_framebuffer_size);

// content scale
CK_DLL_SFUN(gwindow_get_content_scale);

// size limits
CK_DLL_SFUN(gwindow_set_window_size_limits);

// position
CK_DLL_SFUN(gwindow_set_window_pos);
CK_DLL_SFUN(gwindow_window_center);

// title
CK_DLL_SFUN(gwindow_set_title);

// iconify
CK_DLL_SFUN(gwindow_inconify);
CK_DLL_SFUN(gwindow_restore);

// attributes
CK_DLL_SFUN(gwindow_set_attrib_resizable);
CK_DLL_SFUN(gwindow_set_attrib_decorated);
CK_DLL_SFUN(gwindow_set_attrib_floating);
CK_DLL_SFUN(gwindow_set_attrib_transparent);
CK_DLL_SFUN(gwindow_opacity);

void ulib_window_query(Chuck_DL_Query* QUERY)
{
    BEGIN_CLASS("MonitorInfo", "Object");
    DOC_CLASS(
      "Information about the monitor the window is on. Do not instantiate this "
      "class directly, use Gwindow.monitorInfo() instead. This data is NOT "
      "updated in real-time, it is only accurate at the time of the call. If "
      "the user drags the window to another monitor, you will need to call "
      "GWindow.monitorInfo() again to get the updated information.");
    monitor_info_width_offset = MVAR("int", "width", false);
    DOC_VAR("width of the monitor in screen coordinates");
    monitor_info_height_offset = MVAR("int", "height", false);
    DOC_VAR("height of the monitor in screen coordinates");
    monitor_info_refresh_rate_offset = MVAR("int", "refreshRate", false);
    DOC_VAR("refresh rate of the monitor in Hz");
    monitor_info_virtual_pos_offset = MVAR("vec2", "virtualPos", false);
    DOC_VAR(
      "position of the monitor on the virtual desktop, in screen coordinates");
    monitor_info_physical_size_mm_offset = MVAR("vec2", "physicalSize", false);
    DOC_VAR("physical size of the monitor in millimeters");
    monitor_info_content_scale_offset = MVAR("vec2", "contentScale", false);
    DOC_VAR(
      "The content scale is the ratio between the current DPI and the "
      "platform's default DPI.");
    monitor_info_work_area_offset = MVAR("vec4", "workArea", false);
    DOC_VAR(
      " The work area is the area of the monitor not occluded by the taskbar "
      "or other system UI elements. This data is returned as vec4 with "
      "following values: @(xpos, ypos, width, height), in screen coordinates ");
    monitor_info_name_offset = MVAR("string", "name", false);
    DOC_VAR("human-readable name of the monitor");
    END_CLASS(); // MonitorInfo

    // Events ========================================================
    BEGIN_CLASS(CHUGL_EventTypeNames[WINDOW_RESIZE], "Event");
    DOC_CLASS(
      "Event triggered whenever the ChuGL window is resized, either by the "
      "user or programmatically."
      "Don't instantiate directly, use GWindow.resizeEvent() instead");
    END_CLASS(); // WindowResizeEvent

    BEGIN_CLASS(CHUGL_EventTypeNames[WINDOW_CLOSE], "Event");
    DOC_CLASS(
      "Event triggered whenever the user attempts to close the ChuGL window, "
      "useful if GWindow.closeable(false) has been set and you need to "
      "do cleanup (save game, flush logs, etc.) before exiting."
      "Don't instantiate directly, use GWindow.closeEvent() instead");
    END_CLASS(); // WindowCloseEvent

    BEGIN_CLASS(CHUGL_EventTypeNames[CONTENT_SCALE], "Event");
    DOC_CLASS(
      "Event triggered whenever the content scale of the window changes."
      "The content scale is the ratio between the current DPI and the "
      "platform's default DPI."
      "Don't instantiate directly, use GWindow.contentScaleEvent() instead");
    END_CLASS(); // ContentScaleChangedEvent

    // GWindow ========================================================
    BEGIN_CLASS("GWindow", "Object");
    DOC_CLASS("All the properties and methods window management");

    // monitor --------------------------------------------------------
    // SFUN(gwindow_monitor_info, "MonitorInfo", "monitorInfo");
    // DOC_FUNC("Get information about the monitor the window is on");

    // callbacks ------------------------------------------------------
    SFUN(gwindow_close_event, CHUGL_EventTypeNames[WINDOW_CLOSE], "closeEvent");
    DOC_FUNC(
      "Event triggered whenever the user attempts to close the ChuGL window");

    SFUN(gwindow_window_resize_event, CHUGL_EventTypeNames[WINDOW_RESIZE],
         "resizeEvent");
    DOC_FUNC(
      "Event triggered whenever the ChuGL window is resized, either by the "
      "user or programmatically");

    SFUN(gwindow_window_content_scale_event, "Event", "contentScaleEvent");
    DOC_FUNC(
      "Event triggered whenever the content scale of the window changes."
      "The content scale is the ratio between the current DPI and the "
      "platform's default DPI.");

    // closing --------------------------------------------------------
    SFUN(gwindow_set_closeable, "void", "closeable");
    ARG("int", "closeable");
    DOC_FUNC("Enable or disable the window's close button");

    SFUN(gwindow_get_closeable, "int", "closeable");
    DOC_FUNC("Get the current state of the window's close button");

    SFUN(gwindow_close, "void", "close");
    DOC_FUNC("Close the window");

    // size -----------------------------------------------------------
    SFUN(gwindow_fullscreen, "void", "fullscreen");
    DOC_FUNC("Set the window to fullscreen mode");

    SFUN(gwindow_fullscreen_width_height, "void", "fullscreen");
    ARG("int", "width");
    ARG("int", "height");
    DOC_FUNC("Set the window to fullscreen mode with the specified resolution");

    SFUN(gwindow_maximize, "void", "maximize");
    DOC_FUNC("Set the window to windowed fullscreen mode");

    SFUN(gwindow_windowed, "void", "windowed");
    ARG("int", "width");
    ARG("int", "height");
    DOC_FUNC(
      "Set the window to windowed mode with the specified width and height");

    SFUN(gwindow_get_window_size, "vec2", "windowSize");
    DOC_FUNC("Get the window size in screen coordinates");

    SFUN(gwindow_get_framebuffer_size, "vec2", "framebufferSize");
    DOC_FUNC("Get the framebuffer size in pixels");

    // content scale --------------------------------------------------
    SFUN(gwindow_get_content_scale, "vec2", "contentScale");
    DOC_FUNC("Get the content scale of current monitor");

    // size limits ----------------------------------------------------
    SFUN(gwindow_set_window_size_limits, "void", "sizeLimits");
    ARG("int", "minWidth");
    ARG("int", "minHeight");
    ARG("int", "maxWidth");
    ARG("int", "maxHeight");
    ARG("vec2", "aspectRatio");
    DOC_FUNC(
      "Set the window size limits, including min/max size and aspect ratio."
      "To disable a limit, pass 0 for the corresponding argument, or @(0, 0)"
      "to disable fixed aspect ratio");

    // position -------------------------------------------------------
    SFUN(gwindow_set_window_pos, "void", "position");
    ARG("int", "x");
    ARG("int", "y");
    DOC_FUNC("Set the window position in screen coordinates");

    SFUN(gwindow_window_center, "void", "center");
    DOC_FUNC("Center the window on its current monitor");

    // title ----------------------------------------------------------
    SFUN(gwindow_set_title, "void", "title");
    ARG("string", "title");
    DOC_FUNC("Set the window title");

    // iconify --------------------------------------------------------
    SFUN(gwindow_inconify, "void", "iconify");
    DOC_FUNC("Iconify the window");

    SFUN(gwindow_restore, "void", "restore");
    DOC_FUNC("Restore the window from iconified state");

    // attributes -----------------------------------------------------
    SFUN(gwindow_set_attrib_resizable, "void", "resizable");
    ARG("int", "resizable");
    DOC_FUNC(
      "Set the window resizable attribute. Must call before GG.nextFrame(). "
      "Default is true.");

    SFUN(gwindow_set_attrib_decorated, "void", "decorated");
    ARG("int", "decorated");
    DOC_FUNC(
      "Set whether the window has decorations such as a border, a close "
      "widget, etc."
      "Must call before GG.nextFrame(). Default is true.");

    SFUN(gwindow_set_attrib_floating, "void", "floating");
    ARG("int", "floating");
    DOC_FUNC(
      "set whether the specified window is floating, also called topmost "
      "or always-on-top. Must call before GG.nextFrame(). Default is false.");

    SFUN(gwindow_set_attrib_transparent, "void", "transparent");
    ARG("int", "transparent");
    DOC_FUNC(
      "set whether the specified window has a transparent framebuffer, "
      "i.e. the window contents is composited with the background using the "
      "window framebuffer alpha channel."
      "Not supported on all platforms. To enable, call GWindow.transparent() "
      "BEFORE GG.nextFrame() is ever called."
      "If platform supports it, you can change opacity via "
      "GWindow.opacity(float)");

    SFUN(gwindow_opacity, "void", "opacity");
    ARG("float", "opacity");
    DOC_FUNC(
      "Set the window opacity, 0.0 is fully transparent, 1.0 is fully opaque."
      "only works if GWindow.transparent() has been called before "
      "GG.nextFrame()"
      "AND the platform supports transparent framebuffers.");

    END_CLASS(); // GWindow
}

CK_DLL_SFUN(gwindow_monitor_info)
{
    // TODO: how to get app->window from here?
    // probably best to have windowPos callback listener, detect
    // when window changes monitors, and update the monitor info
    // in the ChuGL_Window struct
    // audiothread just reads from the struct
}

// ============================================================================
// callbacks
// ============================================================================

CK_DLL_SFUN(gwindow_close_event)
{
    RETURN->v_object
      = (Chuck_Object*)Event_Get(CHUGL_EventType::WINDOW_CLOSE, API, VM);
}

CK_DLL_SFUN(gwindow_window_resize_event)
{
    RETURN->v_object
      = (Chuck_Object*)Event_Get(CHUGL_EventType::WINDOW_RESIZE, API, VM);
}

CK_DLL_SFUN(gwindow_window_content_scale_event)
{
    RETURN->v_object
      = (Chuck_Object*)Event_Get(CHUGL_EventType::CONTENT_SCALE, API, VM);
}

// ============================================================================
// closing
// ============================================================================

CK_DLL_SFUN(gwindow_set_closeable)
{
    CHUGL_Window_Closeable(GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(gwindow_get_closeable)
{
    RETURN->v_int = CHUGL_Window_Closeable();
}

CK_DLL_SFUN(gwindow_close)
{
    // https://www.glfw.org/docs/3.3/intro_guide.html#thread_safety
    // window creation and destruction, and event processing, are not
    // thread-safe.
    CQ_PushCommand_WindowClose();
}

// ============================================================================
// window size
// ============================================================================

CK_DLL_SFUN(gwindow_fullscreen)
{
    CQ_PushCommand_WindowMode(SG_WINDOW_MODE_FULLSCREEN, 0, 0);
}

CK_DLL_SFUN(gwindow_fullscreen_width_height)
{
    t_CKINT width  = GET_NEXT_INT(ARGS);
    t_CKINT height = GET_NEXT_INT(ARGS);
    CQ_PushCommand_WindowMode(SG_WINDOW_MODE_FULLSCREEN, width, height);
}

CK_DLL_SFUN(gwindow_windowed)
{
    t_CKINT width  = GET_NEXT_INT(ARGS);
    t_CKINT height = GET_NEXT_INT(ARGS);
    CQ_PushCommand_WindowMode(SG_WINDOW_MODE_WINDOWED, width, height);
}

CK_DLL_SFUN(gwindow_maximize)
{
    CQ_PushCommand_WindowMode(SG_WINDOW_MODE_WINDOWED_FULLSCREEN, 0, 0);
}

CK_DLL_SFUN(gwindow_get_window_size)
{
    RETURN->v_vec2 = CHUGL_Window_WindowSize();
}

CK_DLL_SFUN(gwindow_get_framebuffer_size)
{
    RETURN->v_vec2 = CHUGL_Window_FramebufferSize();
}

// ============================================================================
// content scale
// ============================================================================

CK_DLL_SFUN(gwindow_get_content_scale)
{
    RETURN->v_vec2 = CHUGL_Window_ContentScale();
}

// ============================================================================
// size limits
// ============================================================================

CK_DLL_SFUN(gwindow_set_window_size_limits)
{
    t_CKINT min_width     = GET_NEXT_INT(ARGS);
    t_CKINT min_height    = GET_NEXT_INT(ARGS);
    t_CKINT max_width     = GET_NEXT_INT(ARGS);
    t_CKINT max_height    = GET_NEXT_INT(ARGS);
    t_CKVEC2 aspect_ratio = GET_NEXT_VEC2(ARGS);

    CQ_PushCommand_WindowSizeLimits(min_width, min_height, max_width,
                                    max_height, (int)aspect_ratio.x,
                                    (int)aspect_ratio.y);
}

// ============================================================================
// position
// ============================================================================

CK_DLL_SFUN(gwindow_set_window_pos)
{
    t_CKINT x = GET_NEXT_INT(ARGS);
    t_CKINT y = GET_NEXT_INT(ARGS);
    CQ_PushCommand_WindowPosition(x, y);
}

CK_DLL_SFUN(gwindow_window_center)
{
    CQ_PushCommand_WindowCenter();
}

// ============================================================================
// title
// ============================================================================

CK_DLL_SFUN(gwindow_set_title)
{
    CQ_PushCommand_WindowTitle(API->object->str(GET_NEXT_STRING(ARGS)));
}

// ============================================================================
// iconify
// ============================================================================

CK_DLL_SFUN(gwindow_inconify)
{
    CQ_PushCommand_WindowIconify(true);
}

CK_DLL_SFUN(gwindow_restore)
{
    CQ_PushCommand_WindowIconify(false);
}

// ============================================================================
// attributes
// ============================================================================

CK_DLL_SFUN(gwindow_set_attrib_resizable)
{
    // CQ_PushCommand_WindowAttribute(CHUGL_WINDOW_ATTRIB_RESIZABLE,
    //                                GET_NEXT_INT(ARGS));
    CHUGL_Window_Resizable(GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(gwindow_set_attrib_decorated)
{
    // CQ_PushCommand_WindowAttribute(CHUGL_WINDOW_ATTRIB_DECORATED,
    //                                GET_NEXT_INT(ARGS));
    CHUGL_Window_Decorated(GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(gwindow_set_attrib_floating)
{
    // CQ_PushCommand_WindowAttribute(CHUGL_WINDOW_ATTRIB_FLOATING,
    //                                GET_NEXT_INT(ARGS));
    CHUGL_Window_Floating(GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(gwindow_set_attrib_transparent)
{
    CHUGL_Window_Transparent(GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(gwindow_opacity)
{
    CQ_PushCommand_WindowOpacity(GET_NEXT_FLOAT(ARGS));
}
