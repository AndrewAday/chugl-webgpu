// exploratory handwritten binding for dear imgui
// hopefully can figure out how to generate this automatically

#include <iostream>
#include <unordered_map>

// Wrap this in a namespace to keep it separate from the C++ API
namespace cimgui
{
#include <cimgui/cimgui.h>
}

#include "ulib_helper.h"

// ============================================================================
// Declarations
// ============================================================================

// Demo, Debug, Information
CK_DLL_SFUN(ui_ShowDemoWindow);
CK_DLL_SFUN(ui_ShowMetricsWindow);
CK_DLL_SFUN(ui_ShowDebugLogWindow);

// Windows
CK_DLL_SFUN(ui_begin);
CK_DLL_SFUN(ui_end);

// Child Windows
CK_DLL_SFUN(ui_BeginChild);
CK_DLL_SFUN(ui_BeginChildID);
CK_DLL_SFUN(ui_EndChild);

// Window manipulation
CK_DLL_SFUN(ui_SetNextWindowPos);
CK_DLL_SFUN(ui_SetNextWindowPosEx);
CK_DLL_SFUN(ui_SetNextWindowSize);
CK_DLL_SFUN(ui_SetNextWindowSizeConstraints);
CK_DLL_SFUN(ui_SetNextWindowContentSize);
CK_DLL_SFUN(ui_SetNextWindowCollapsed);
CK_DLL_SFUN(ui_SetNextWindowFocus);
CK_DLL_SFUN(ui_SetNextWindowScroll);
CK_DLL_SFUN(ui_SetNextWindowBgAlpha);
CK_DLL_SFUN(ui_SetNextWindowViewport);

// Widgets: Main
CK_DLL_SFUN(ui_button);
CK_DLL_SFUN(ui_checkbox);

// Styles
CK_DLL_SFUN(ui_styleColorsDark);
CK_DLL_SFUN(ui_styleColorsLight);
CK_DLL_SFUN(ui_styleColorsClassic);
CK_DLL_SFUN(ui_showStyleEditor);

// Callbacks -----------------------------------------------------------------

// ImGuiSizeCallback
CK_DLL_MFUN(ui_size_callback);
CK_DLL_CTOR(ui_size_callback_ctor);
CK_DLL_DTOR(ui_size_callback_dtor);
static t_CKINT ui_size_callback_vt_offset = -1;
// ImGuiSizeCallbackData
static t_CKUINT ui_size_callback_data_ptr_offset = 0;
CK_DLL_MFUN(ui_size_callback_data_get_pos);
CK_DLL_MFUN(ui_size_callback_data_get_current_size);
CK_DLL_MFUN(ui_size_callback_data_get_desired_size);
CK_DLL_MFUN(ui_size_callback_data_set_desired_size);

// UI_Bool
static t_CKUINT ui_bool_ptr_offset = 0;
CK_DLL_CTOR(ui_bool_ctor);
CK_DLL_DTOR(ui_bool_dtor);
CK_DLL_MFUN(ui_bool_get_value);
CK_DLL_MFUN(ui_bool_set_value);

// UI_Style

void ulib_imgui_query(Chuck_DL_Query* QUERY)
{
    // UI_Bool ---------------------------------------------------------------
    QUERY->begin_class(QUERY, "UI_Bool", "Object");
    ui_bool_ptr_offset = QUERY->add_mvar(QUERY, "int", "@ui_bool_ptr", false);

    QUERY->add_ctor(QUERY, ui_bool_ctor);
    QUERY->add_dtor(QUERY, ui_bool_dtor);
    QUERY->add_mfun(QUERY, ui_bool_get_value, "int", "val");
    QUERY->add_mfun(QUERY, ui_bool_set_value, "int", "val");
    QUERY->add_arg(QUERY, "int", "val");

    QUERY->end_class(QUERY); // UI_Bool

    // enums
    QUERY->begin_class(QUERY, "UI_WindowFlags", "Object");
    QUERY->doc_class(
      QUERY,
      "Flags for ImGui::Begin().\n(Those are per-window flags. There are "
      "shared flags in ImGuiIO: io.ConfigWindowsResizeFromEdges and "
      "io.ConfigWindowsMoveFromTitleBarOnly).\n");
    static t_CKINT ImGuiWindowFlags_None = 0;
    QUERY->add_svar(QUERY, "int", "None", true, &ImGuiWindowFlags_None);
    static t_CKINT ImGuiWindowFlags_NoTitleBar = 1;
    QUERY->add_svar(QUERY, "int", "NoTitleBar", true,
                    &ImGuiWindowFlags_NoTitleBar);
    QUERY->doc_var(QUERY, "Disable title-bar");
    static t_CKINT ImGuiWindowFlags_NoResize = 2;
    QUERY->add_svar(QUERY, "int", "NoResize", true, &ImGuiWindowFlags_NoResize);
    QUERY->doc_var(QUERY, "Disable user resizing with the lower-right grip");
    static t_CKINT ImGuiWindowFlags_NoMove = 4;
    QUERY->add_svar(QUERY, "int", "NoMove", true, &ImGuiWindowFlags_NoMove);
    QUERY->doc_var(QUERY, "Disable user moving the window");
    static t_CKINT ImGuiWindowFlags_NoScrollbar = 8;
    QUERY->add_svar(QUERY, "int", "NoScrollbar", true,
                    &ImGuiWindowFlags_NoScrollbar);
    QUERY->doc_var(QUERY,
                   "Disable scrollbars (window can still scroll with mouse or "
                   "programmatically)");
    static t_CKINT ImGuiWindowFlags_NoScrollWithMouse = 16;
    QUERY->add_svar(QUERY, "int", "NoScrollWithMouse", true,
                    &ImGuiWindowFlags_NoScrollWithMouse);
    QUERY->doc_var(QUERY,
                   "Disable user vertically scrolling with mouse wheel. On "
                   "child window, mouse wheel will be forwarded to the parent "
                   "unless NoScrollbar is also set.");
    static t_CKINT ImGuiWindowFlags_NoCollapse = 32;
    QUERY->add_svar(QUERY, "int", "NoCollapse", true,
                    &ImGuiWindowFlags_NoCollapse);
    QUERY->doc_var(
      QUERY,
      "Disable user collapsing window by double-clicking on it. Also referred "
      "to as Window Menu Button (e.g. within a docking node).");
    static t_CKINT ImGuiWindowFlags_AlwaysAutoResize = 64;
    QUERY->add_svar(QUERY, "int", "AlwaysAutoResize", true,
                    &ImGuiWindowFlags_AlwaysAutoResize);
    QUERY->doc_var(QUERY, "Resize every window to its content every frame");
    static t_CKINT ImGuiWindowFlags_NoBackground = 128;
    QUERY->add_svar(QUERY, "int", "NoBackground", true,
                    &ImGuiWindowFlags_NoBackground);
    QUERY->doc_var(
      QUERY,
      "Disable drawing background color (WindowBg, etc.) and outside border. "
      "Similar as using SetNextWindowBgAlpha(0.0f).");
    static t_CKINT ImGuiWindowFlags_NoSavedSettings = 256;
    QUERY->add_svar(QUERY, "int", "NoSavedSettings", true,
                    &ImGuiWindowFlags_NoSavedSettings);
    QUERY->doc_var(QUERY, "Never load/save settings in .ini file");
    static t_CKINT ImGuiWindowFlags_NoMouseInputs = 512;
    QUERY->add_svar(QUERY, "int", "NoMouseInputs", true,
                    &ImGuiWindowFlags_NoMouseInputs);
    QUERY->doc_var(QUERY,
                   "Disable catching mouse, hovering test with pass through.");
    static t_CKINT ImGuiWindowFlags_MenuBar = 1024;
    QUERY->add_svar(QUERY, "int", "MenuBar", true, &ImGuiWindowFlags_MenuBar);
    QUERY->doc_var(QUERY, "Has a menu-bar");
    static t_CKINT ImGuiWindowFlags_HorizontalScrollbar = 2048;
    QUERY->add_svar(QUERY, "int", "HorizontalScrollbar", true,
                    &ImGuiWindowFlags_HorizontalScrollbar);
    QUERY->doc_var(QUERY,
                   "Allow horizontal scrollbar to appear (off by default). You "
                   "may use SetNextWindowContentSize(ImVec2(width,0.0f)); "
                   "prior to calling Begin() to specify width. Read code in "
                   "imgui_demo in the \"Horizontal Scrolling\" section.");
    static t_CKINT ImGuiWindowFlags_NoFocusOnAppearing = 4096;
    QUERY->add_svar(QUERY, "int", "NoFocusOnAppearing", true,
                    &ImGuiWindowFlags_NoFocusOnAppearing);
    QUERY->doc_var(
      QUERY,
      "Disable taking focus when transitioning from hidden to visible state");
    static t_CKINT ImGuiWindowFlags_NoBringToFrontOnFocus = 8192;
    QUERY->add_svar(QUERY, "int", "NoBringToFrontOnFocus", true,
                    &ImGuiWindowFlags_NoBringToFrontOnFocus);
    QUERY->doc_var(QUERY,
                   "Disable bringing window to front when taking focus (e.g. "
                   "clicking on it or programmatically giving it focus)");
    static t_CKINT ImGuiWindowFlags_AlwaysVerticalScrollbar = 16384;
    QUERY->add_svar(QUERY, "int", "AlwaysVerticalScrollbar", true,
                    &ImGuiWindowFlags_AlwaysVerticalScrollbar);
    QUERY->doc_var(
      QUERY, "Always show vertical scrollbar (even if ContentSize.y < Size.y)");
    static t_CKINT ImGuiWindowFlags_AlwaysHorizontalScrollbar = 32768;
    QUERY->add_svar(QUERY, "int", "AlwaysHorizontalScrollbar", true,
                    &ImGuiWindowFlags_AlwaysHorizontalScrollbar);
    QUERY->doc_var(
      QUERY,
      "Always show horizontal scrollbar (even if ContentSize.x < Size.x)");
    static t_CKINT ImGuiWindowFlags_NoNavInputs = 65536;
    QUERY->add_svar(QUERY, "int", "NoNavInputs", true,
                    &ImGuiWindowFlags_NoNavInputs);
    QUERY->doc_var(QUERY, "No gamepad/keyboard navigation within the window");
    static t_CKINT ImGuiWindowFlags_NoNavFocus = 131072;
    QUERY->add_svar(QUERY, "int", "NoNavFocus", true,
                    &ImGuiWindowFlags_NoNavFocus);
    QUERY->doc_var(QUERY,
                   "No focusing toward this window with gamepad/keyboard "
                   "navigation (e.g. skipped by CTRL+TAB)");
    static t_CKINT ImGuiWindowFlags_UnsavedDocument = 262144;
    QUERY->add_svar(QUERY, "int", "UnsavedDocument", true,
                    &ImGuiWindowFlags_UnsavedDocument);
    QUERY->doc_var(
      QUERY,
      "Display a dot next to the title. When used in a tab/docking context, "
      "tab is selected when clicking the X + closure is not assumed (will wait "
      "for user to stop submitting the tab). Otherwise closure is assumed when "
      "pressing the X, so if you keep submitting the tab may reappear at end "
      "of tab bar.");
    static t_CKINT ImGuiWindowFlags_NoDocking = 524288;
    QUERY->add_svar(QUERY, "int", "NoDocking", true,
                    &ImGuiWindowFlags_NoDocking);
    QUERY->doc_var(QUERY, "Disable docking of this window");
    static t_CKINT ImGuiWindowFlags_NoNav = 196608;
    QUERY->add_svar(QUERY, "int", "NoNav", true, &ImGuiWindowFlags_NoNav);
    static t_CKINT ImGuiWindowFlags_NoDecoration = 43;
    QUERY->add_svar(QUERY, "int", "NoDecoration", true,
                    &ImGuiWindowFlags_NoDecoration);
    static t_CKINT ImGuiWindowFlags_NoInputs = 197120;
    QUERY->add_svar(QUERY, "int", "NoInputs", true, &ImGuiWindowFlags_NoInputs);
    QUERY->end_class(QUERY);

    QUERY->begin_class(QUERY, "UI_ChildFlags", "Object");
    QUERY->doc_class(
      QUERY,
      "Flags for ImGui::BeginChild().\n(Legacy: bit 0 must always correspond "
      "to ImGuiChildFlags_Border to be backward compatible with old API using "
      "'bool border = false'..\nAbout using AutoResizeX/AutoResizeY flags:.\n- "
      "May be combined with SetNextWindowSizeConstraints() to set a min/max "
      "size for each axis (see \"Demo->Child->Auto-resize with "
      "Constraints\")..\n- Size measurement for a given axis is only performed "
      "when the child window is within visible boundaries, or is just "
      "appearing..\n- This allows BeginChild() to return false when not within "
      "boundaries (e.g. when scrolling), which is more optimal. BUT it won't "
      "update its auto-size while clipped..\nWhile not perfect, it is a better "
      "default behavior as the always-on performance gain is more valuable "
      "than the occasional \"resizing after becoming visible again\" "
      "glitch..\n- You may also use ImGuiChildFlags_AlwaysAutoResize to force "
      "an update even when child window is not in view..\nHOWEVER PLEASE "
      "UNDERSTAND THAT DOING SO WILL PREVENT BeginChild() FROM EVER RETURNING "
      "FALSE, disabling benefits of coarse clipping..\n");
    static t_CKINT ImGuiChildFlags_None = 0;
    QUERY->add_svar(QUERY, "int", "None", true, &ImGuiChildFlags_None);
    static t_CKINT ImGuiChildFlags_Border = 1;
    QUERY->add_svar(QUERY, "int", "Border", true, &ImGuiChildFlags_Border);
    QUERY->doc_var(QUERY,
                   "Show an outer border and enable WindowPadding. (IMPORTANT: "
                   "this is always == 1 == true for legacy reason)");
    static t_CKINT ImGuiChildFlags_AlwaysUseWindowPadding = 2;
    QUERY->add_svar(QUERY, "int", "AlwaysUseWindowPadding", true,
                    &ImGuiChildFlags_AlwaysUseWindowPadding);
    QUERY->doc_var(
      QUERY,
      "Pad with style.WindowPadding even if no border are drawn (no padding by "
      "default for non-bordered child windows because it makes more sense)");
    static t_CKINT ImGuiChildFlags_ResizeX = 4;
    QUERY->add_svar(QUERY, "int", "ResizeX", true, &ImGuiChildFlags_ResizeX);
    QUERY->doc_var(
      QUERY,
      "Allow resize from right border (layout direction). Enable .ini saving "
      "(unless ImGuiWindowFlags_NoSavedSettings passed to window flags)");
    static t_CKINT ImGuiChildFlags_ResizeY = 8;
    QUERY->add_svar(QUERY, "int", "ResizeY", true, &ImGuiChildFlags_ResizeY);
    QUERY->doc_var(QUERY,
                   "Allow resize from bottom border (layout direction). \"");
    static t_CKINT ImGuiChildFlags_AutoResizeX = 16;
    QUERY->add_svar(QUERY, "int", "AutoResizeX", true,
                    &ImGuiChildFlags_AutoResizeX);
    QUERY->doc_var(QUERY,
                   "Enable auto-resizing width. Read \"IMPORTANT: Size "
                   "measurement\" details above.");
    static t_CKINT ImGuiChildFlags_AutoResizeY = 32;
    QUERY->add_svar(QUERY, "int", "AutoResizeY", true,
                    &ImGuiChildFlags_AutoResizeY);
    QUERY->doc_var(QUERY,
                   "Enable auto-resizing height. Read \"IMPORTANT: Size "
                   "measurement\" details above.");
    static t_CKINT ImGuiChildFlags_AlwaysAutoResize = 64;
    QUERY->add_svar(QUERY, "int", "AlwaysAutoResize", true,
                    &ImGuiChildFlags_AlwaysAutoResize);
    QUERY->doc_var(QUERY,
                   "Combined with AutoResizeX/AutoResizeY. Always measure size "
                   "even when child is hidden, always return true, always "
                   "disable clipping optimization! NOT RECOMMENDED.");
    static t_CKINT ImGuiChildFlags_FrameStyle = 128;
    QUERY->add_svar(QUERY, "int", "FrameStyle", true,
                    &ImGuiChildFlags_FrameStyle);
    QUERY->doc_var(QUERY,
                   "Style the child window like a framed item: use FrameBg, "
                   "FrameRounding, FrameBorderSize, FramePadding instead of "
                   "ChildBg, ChildRounding, ChildBorderSize, WindowPadding.");
    QUERY->end_class(QUERY);

    QUERY->begin_class(QUERY, "UI_Cond", "Object");
    QUERY->doc_class(
      QUERY,
      "Enumeration for ImGui::SetNextWindow***(), SetWindow***(), "
      "SetNextItem***() functions.\nRepresent a condition..\nImportant: Treat "
      "as a regular enum! Do NOT combine multiple values using binary "
      "operators! All the functions above treat 0 as a shortcut to "
      "ImGuiCond_Always..\n");
    static t_CKINT ImGuiCond_None = 0;
    QUERY->add_svar(QUERY, "int", "None", true, &ImGuiCond_None);
    QUERY->doc_var(QUERY,
                   "No condition (always set the variable), same as _Always");
    static t_CKINT ImGuiCond_Always = 1;
    QUERY->add_svar(QUERY, "int", "Always", true, &ImGuiCond_Always);
    QUERY->doc_var(QUERY,
                   "No condition (always set the variable), same as _None");
    static t_CKINT ImGuiCond_Once = 2;
    QUERY->add_svar(QUERY, "int", "Once", true, &ImGuiCond_Once);
    QUERY->doc_var(QUERY,
                   "Set the variable once per runtime session (only the first "
                   "call will succeed)");
    static t_CKINT ImGuiCond_FirstUseEver = 4;
    QUERY->add_svar(QUERY, "int", "FirstUseEver", true,
                    &ImGuiCond_FirstUseEver);
    QUERY->doc_var(QUERY,
                   "Set the variable if the object/window has no persistently "
                   "saved data (no entry in .ini file)");
    static t_CKINT ImGuiCond_Appearing = 8;
    QUERY->add_svar(QUERY, "int", "Appearing", true, &ImGuiCond_Appearing);
    QUERY->doc_var(QUERY,
                   "Set the variable if the object/window is appearing after "
                   "being hidden/inactive (or the first time)");
    QUERY->end_class(QUERY);

    // Callbacks ----------------------------------------------------------

    QUERY->begin_class(QUERY, "UI_SizeCallbackData", "Object");
    ui_size_callback_data_ptr_offset
      = QUERY->add_mvar(QUERY, "int", "@ui_size_callback_data_ptr", false);
    // QUERY->add_mvar(QUERY, "int", "user_data", false);  // IGNORE user data
    // (chuck provides other ways to access external data)
    QUERY->add_mfun(QUERY, ui_size_callback_data_get_pos, "vec2", "getPos");
    QUERY->doc_func(QUERY, "Read-only window position, for reference.");
    QUERY->add_mfun(QUERY, ui_size_callback_data_get_current_size, "vec2",
                    "currentSize");
    QUERY->doc_func(QUERY, "Read-only current window size.");
    QUERY->add_mfun(QUERY, ui_size_callback_data_get_desired_size, "vec2",
                    "desiredSize");
    QUERY->doc_func(QUERY,
                    "Read-write desired size, based on user's mouse "
                    "position. Write to this field to restrain resizing.");
    QUERY->add_mfun(QUERY, ui_size_callback_data_set_desired_size, "vec2",
                    "desiredSize");
    QUERY->add_arg(QUERY, "vec2", "desired_size");
    QUERY->doc_func(QUERY,
                    "Write desired size, based on user's mouse "
                    "position. Write to this field to restrain resizing.");
    QUERY->end_class(QUERY);

    QUERY->begin_class(QUERY, "UI_SizeCallback", "Object");
    QUERY->add_ctor(QUERY, ui_size_callback_ctor);
    QUERY->add_dtor(QUERY, ui_size_callback_dtor);
    QUERY->add_mfun(QUERY, ui_size_callback, "void", "handler");
    QUERY->add_arg(QUERY, "UI_SizeCallbackData", "data");
    QUERY->doc_func(
      QUERY, "Callback function for ImGui::SetNextWindowSizeConstraints()");
    QUERY->end_class(QUERY);

    // update() vt offset
    chugin_setVTableOffset(&ui_size_callback_vt_offset, "UI_SizeCallback",
                           "handler");
    // Chuck_Type* ui_size_callback_t
    //   = g_chuglAPI->type->lookup(g_chuglVM, "UI_SizeCallback");
    // // find the offset for update
    // ui_size_callback_vt_offset = g_chuglAPI->type->get_vtable_offset(
    //   g_chuglVM, ui_size_callback_t, "handler");

    // UI ---------------------------------------------------------------------
    QUERY->begin_class(QUERY, "UI", "Object");

    // Demo, Debug, Information
    QUERY->add_sfun(QUERY, ui_ShowDemoWindow, "void", "showDemoWindow");
    QUERY->add_arg(QUERY, "UI_Bool", "p_open");
    QUERY->doc_func(QUERY,
                    "create Demo window. demonstrate most ImGui features. call "
                    "this to learn about the library! try to make it always "
                    "available in your application!");
    QUERY->add_sfun(QUERY, ui_ShowMetricsWindow, "void", "showMetricsWindow");
    QUERY->add_arg(QUERY, "UI_Bool", "p_open");
    QUERY->doc_func(
      QUERY,
      "create Metrics/Debugger window. display Dear ImGui internals: windows, "
      "draw commands, various internal state, etc.");
    QUERY->add_sfun(QUERY, ui_ShowDebugLogWindow, "void", "showDebugLogWindow");
    QUERY->add_arg(QUERY, "UI_Bool", "p_open");
    QUERY->doc_func(QUERY,
                    "create Debug Log window. display a simplified log of "
                    "important dear imgui events.");
    QUERY->add_sfun(QUERY, ui_showStyleEditor, "void", "showStyleEditor");
    QUERY->doc_func(QUERY,
                    "add style selector block (not a window), essentially a "
                    "combo listing the default styles.");

    // Windows
    QUERY->add_sfun(QUERY, ui_begin, "int", "begin");
    QUERY->add_arg(QUERY, "string", "name");
    QUERY->add_arg(QUERY, "UI_Bool", "p_open");
    QUERY->add_arg(QUERY, "int", "flags"); // ImGuiWindowFlags

    QUERY->add_sfun(QUERY, ui_end, "void", "end");

    // Child windows
    QUERY->add_sfun(QUERY, ui_BeginChild, "int", "beginChild");
    QUERY->add_arg(QUERY, "string", "str_id");
    QUERY->add_arg(QUERY, "vec2", "size");        // map ImVec2 --> vec2
    QUERY->add_arg(QUERY, "int", "child_flags");  // ImGuiChildFlags
    QUERY->add_arg(QUERY, "int", "window_flags"); // ImGuiWindowFlags

    QUERY->add_sfun(QUERY, ui_EndChild, "void", "endChild");

    // Window manipulation
    QUERY->add_sfun(QUERY, ui_SetNextWindowPos, "void", "setNextWindowPos");
    QUERY->add_arg(QUERY, "vec2", "pos");
    QUERY->add_arg(QUERY, "int", "cond" /*ImGuiCond*/);
    QUERY->doc_func(QUERY, "Implied pivot = ImVec2(0, 0)");

    QUERY->add_sfun(QUERY, ui_SetNextWindowPosEx, "void", "setNextWindowPosEx");
    QUERY->add_arg(QUERY, "vec2", "pos");
    QUERY->add_arg(QUERY, "int", "cond");
    QUERY->add_arg(QUERY, "vec2", "pivot");
    QUERY->doc_func(QUERY,
                    "set next window position. call before Begin(). "
                    "use pivot=(0.5f,0.5f) to center on given point, etc.");

    QUERY->add_sfun(QUERY, ui_SetNextWindowSize, "void", "setNextWindowSize");
    QUERY->add_arg(QUERY, "vec2", "size");
    QUERY->add_arg(QUERY, "int", "cond");
    QUERY->doc_func(QUERY,
                    "set next window size. set axis to 0.0f to force an "
                    "auto-fit on this axis. call before Begin()");

    QUERY->add_sfun(QUERY, ui_SetNextWindowSizeConstraints, "void",
                    "setNextWindowSizeConstraints");
    QUERY->add_arg(QUERY, "vec2", "size_min");
    QUERY->add_arg(QUERY, "vec2", "size_max");
    QUERY->add_arg(QUERY, "UI_SizeCallback", "custom_callback");
    QUERY->doc_func(
      QUERY,
      "set next window size limits. use 0.0f or FLT_MAX if you "
      "don't want limits. Use -1 for both min and max of same "
      "axis to preserve current size (which itself is a "
      "constraint). Use callback to apply non-trivial programmatic "
      "constraints.");

    QUERY->add_sfun(QUERY, ui_SetNextWindowContentSize, "void",
                    "setNextWindowContentSize");
    QUERY->add_arg(QUERY, "vec2", "size");
    QUERY->doc_func(
      QUERY,
      "set next window content size (~ scrollable client area, which enforce "
      "the range of scrollbars). Not including window decorations (title bar, "
      "menu bar, etc.) nor WindowPadding. set an axis to 0.0f to leave it "
      "automatic. call before Begin()");

    QUERY->add_sfun(QUERY, ui_SetNextWindowCollapsed, "void",
                    "setNextWindowCollapsed");
    QUERY->add_arg(QUERY, "int", "collapsed");
    QUERY->add_arg(QUERY, "int", "cond" /*ImGuiCond*/);
    QUERY->doc_func(QUERY,
                    "set next window collapsed state. call before Begin()");

    QUERY->add_sfun(QUERY, ui_SetNextWindowFocus, "void", "setNextWindowFocus");
    QUERY->doc_func(
      QUERY, "set next window to be focused / top-most. call before Begin()");

    QUERY->add_sfun(QUERY, ui_SetNextWindowScroll, "void",
                    "setNextWindowScroll");
    QUERY->add_arg(QUERY, "vec2", "scroll");
    QUERY->doc_func(QUERY,
                    "set next window scrolling value (use < 0.0f to not affect "
                    "a given axis).");

    QUERY->add_sfun(QUERY, ui_SetNextWindowBgAlpha, "void",
                    "setNextWindowBgAlpha");
    QUERY->add_arg(QUERY, "float", "alpha");
    QUERY->doc_func(
      QUERY,
      "set next window background color alpha. helper to easily override the "
      "Alpha component of ImGuiCol_WindowBg/ChildBg/PopupBg. you may also use "
      "ImGuiWindowFlags_NoBackground.");

    // multiple viewports currently unsupported in webgpu
    // Being added in: https://github.com/ocornut/imgui/pull/7557
    // TODO: after viewport support is added for webgpu native, impl this
    // binding QUERY->add_sfun(QUERY, ui_SetNextWindowViewport, "void",
    //                 "setNextWindowViewport");
    // QUERY->add_arg(QUERY, "int", "viewport_id");
    // QUERY->doc_func(QUERY, "set next window viewport");

    // Widgets: Main
    QUERY->add_sfun(QUERY, ui_button, "int", "button");
    QUERY->add_arg(QUERY, "string", "label");

    QUERY->add_sfun(QUERY, ui_checkbox, "int", "checkbox");
    QUERY->add_arg(QUERY, "string", "label");
    QUERY->add_arg(QUERY, "UI_Bool", "bool");

    // styles
    QUERY->add_sfun(QUERY, ui_styleColorsDark, "void", "styleColorsDark");
    QUERY->add_sfun(QUERY, ui_styleColorsLight, "void", "styleColorsLight");
    QUERY->add_sfun(QUERY, ui_styleColorsClassic, "void", "styleColorsClassic");

    QUERY->end_class(QUERY); // UI
}

// ============================================================================
// Demo, Debug, Information
// ============================================================================
CK_DLL_SFUN(ui_ShowDemoWindow)
{
    bool* p_open
      = (bool*)OBJ_MEMBER_UINT(GET_NEXT_OBJECT(ARGS), ui_bool_ptr_offset);
    cimgui::ImGui_ShowDemoWindow(p_open);
}

CK_DLL_SFUN(ui_ShowMetricsWindow)
{
    bool* p_open
      = (bool*)OBJ_MEMBER_UINT(GET_NEXT_OBJECT(ARGS), ui_bool_ptr_offset);
    cimgui::ImGui_ShowMetricsWindow(p_open);
}

CK_DLL_SFUN(ui_ShowDebugLogWindow)
{
    bool* p_open
      = (bool*)OBJ_MEMBER_UINT(GET_NEXT_OBJECT(ARGS), ui_bool_ptr_offset);
    cimgui::ImGui_ShowDebugLogWindow(p_open);
}

CK_DLL_SFUN(ui_showStyleEditor)
{
    cimgui::ImGui_ShowStyleEditor(NULL);
}

// ============================================================================
// structs
// ============================================================================

// UI_Bool -------------------------------------------------------------------
CK_DLL_CTOR(ui_bool_ctor)
{
    bool* b                                   = new bool(false);
    OBJ_MEMBER_UINT(SELF, ui_bool_ptr_offset) = (t_CKUINT)b;
}

CK_DLL_DTOR(ui_bool_dtor)
{
    bool* b = (bool*)OBJ_MEMBER_UINT(SELF, ui_bool_ptr_offset);
    delete b;
    OBJ_MEMBER_UINT(SELF, ui_bool_ptr_offset) = 0;
}

CK_DLL_MFUN(ui_bool_get_value)
{
    bool* b       = (bool*)OBJ_MEMBER_UINT(SELF, ui_bool_ptr_offset);
    RETURN->v_int = *b;
}

CK_DLL_MFUN(ui_bool_set_value)
{
    bool* b       = (bool*)OBJ_MEMBER_UINT(SELF, ui_bool_ptr_offset);
    *b            = (bool)GET_NEXT_INT(ARGS);
    RETURN->v_int = *b;
}

CK_DLL_SFUN(ui_begin)
{
    const char* name      = API->object->str(GET_NEXT_STRING(ARGS));
    Chuck_Object* ui_bool = GET_NEXT_OBJECT(ARGS);
    bool* p_open
      = ui_bool ? (bool*)OBJ_MEMBER_UINT(ui_bool, ui_bool_ptr_offset) : NULL;
    int flags = GET_NEXT_INT(ARGS);

    bool ret      = cimgui::ImGui_Begin(name, p_open, flags);
    RETURN->v_int = ret;
}

CK_DLL_SFUN(ui_end)
{
    cimgui::ImGui_End();
}

// ============================================================================
// Child Windows
// ============================================================================
CK_DLL_SFUN(ui_BeginChild)
{

    const char* str_id = API->object->str(GET_NEXT_STRING(ARGS));
    t_CKVEC2 size      = GET_NEXT_VEC2(ARGS);
    int child_flags    = GET_NEXT_INT(ARGS);
    int window_flags   = GET_NEXT_INT(ARGS);

    cimgui::ImGui_BeginChild(str_id, { (float)size.x, (float)size.y },
                             child_flags, window_flags);
}

CK_DLL_SFUN(ui_EndChild)
{
    cimgui::ImGui_EndChild();
}

// ============================
// Window Manipulation
// ============================
CK_DLL_SFUN(ui_SetNextWindowPos)
{
    t_CKVEC2 pos = GET_NEXT_VEC2(ARGS);
    t_CKINT cond = GET_NEXT_INT(ARGS);
    cimgui::ImGui_SetNextWindowPos({ (float)pos.x, (float)pos.y }, cond);
}

CK_DLL_SFUN(ui_SetNextWindowPosEx)
{
}
CK_DLL_SFUN(ui_SetNextWindowSize)
{
}

static void uiSizeCallbackHandler(cimgui::ImGuiSizeCallbackData* data)
{
    std::cout << "uisizecallbackhandler" << std::endl;
    // TODO put g_chuglAPI and g_chuglVM in shared header
    static Chuck_VM* VM  = g_chuglVM;
    static CK_DL_API API = g_chuglAPI;

    Chuck_Object* ui_size_callback_ckobj = (Chuck_Object*)data->UserData;
    // must use shred associated with UI_SizeCallback instance
    Chuck_VM_Shred* origin_shred
      = chugin_getOriginShred(ui_size_callback_ckobj);

    Chuck_Object* size_callback_data = API->object->create_without_shred(
      VM, API->type->lookup(VM, "UI_SizeCallbackData"), false);
    OBJ_MEMBER_INT(size_callback_data, ui_size_callback_data_ptr_offset)
      = (t_CKINT)data;

    // put callback data into an argument
    Chuck_DL_Arg theArg;
    theArg.kind           = kindof_INT;
    theArg.value.v_object = size_callback_data;

    // invoke the update function in immediate mode
    API->vm->invoke_mfun_immediate_mode(ui_size_callback_ckobj,
                                        ui_size_callback_vt_offset, VM,
                                        origin_shred, &theArg, 1);
}

CK_DLL_SFUN(ui_SetNextWindowSizeConstraints)
{
    t_CKVEC2 size_min              = GET_NEXT_VEC2(ARGS);
    t_CKVEC2 size_max              = GET_NEXT_VEC2(ARGS);
    Chuck_Object* ui_size_callback = GET_NEXT_OBJECT(ARGS);

    // anonymous callback function
    cimgui::ImGui_SetNextWindowSizeConstraints(
      { (float)size_min.x, (float)size_min.y },
      { (float)size_max.x, (float)size_max.y }, uiSizeCallbackHandler,
      ui_size_callback);
}

CK_DLL_SFUN(ui_SetNextWindowContentSize)
{
    t_CKVEC2 size = GET_NEXT_VEC2(ARGS);
    cimgui::ImGui_SetNextWindowContentSize({ (float)size.x, (float)size.y });
}

CK_DLL_SFUN(ui_SetNextWindowCollapsed)
{
    t_CKINT collapsed = GET_NEXT_INT(ARGS);
    t_CKINT cond      = GET_NEXT_INT(ARGS);
    cimgui::ImGui_SetNextWindowCollapsed(collapsed, cond);
}

CK_DLL_SFUN(ui_SetNextWindowFocus)
{
    cimgui::ImGui_SetNextWindowFocus();
}

CK_DLL_SFUN(ui_SetNextWindowScroll)
{
    t_CKVEC2 scroll = GET_NEXT_VEC2(ARGS);
    cimgui::ImGui_SetNextWindowScroll({ (float)scroll.x, (float)scroll.y });
}

CK_DLL_SFUN(ui_SetNextWindowBgAlpha)
{
    t_CKFLOAT alpha = GET_NEXT_FLOAT(ARGS);
    cimgui::ImGui_SetNextWindowBgAlpha(alpha);
}

// waiting for multiple viewports support in webgpu
// CK_DLL_SFUN(ui_SetNextWindowViewport)
// {
//    t_CKINT viewport_id = GET_NEXT_INT(ARGS);
//    cimgui::ImGui_SetNextWindowViewport(viewport_id);
// }

// ============================================================================
// Callbacks
// ============================================================================

CK_DLL_CTOR(ui_size_callback_ctor)
{
    // TODO prob make all callback classes extend "UI_Callback" base and
    // do this in the shared ctor
    chugin_setOriginShred(SELF, SHRED);
}
CK_DLL_DTOR(ui_size_callback_dtor)
{
    // TODO same for dtor as constructor
    chugin_removeFromOriginShredMap(SELF);
}

// user defined callback
CK_DLL_MFUN(ui_size_callback)
{
}

CK_DLL_MFUN(ui_size_callback_data_get_pos)
{
    cimgui::ImGuiSizeCallbackData* data
      = (cimgui::ImGuiSizeCallbackData*)OBJ_MEMBER_UINT(
        SELF, ui_size_callback_data_ptr_offset);
    RETURN->v_vec2 = { data->Pos.x, data->Pos.y };
}

CK_DLL_MFUN(ui_size_callback_data_get_current_size)
{
    cimgui::ImGuiSizeCallbackData* data
      = (cimgui::ImGuiSizeCallbackData*)OBJ_MEMBER_UINT(
        SELF, ui_size_callback_data_ptr_offset);
    RETURN->v_vec2 = { data->CurrentSize.x, data->CurrentSize.y };
}

CK_DLL_MFUN(ui_size_callback_data_get_desired_size)
{
    cimgui::ImGuiSizeCallbackData* data
      = (cimgui::ImGuiSizeCallbackData*)OBJ_MEMBER_UINT(
        SELF, ui_size_callback_data_ptr_offset);
    RETURN->v_vec2 = { data->DesiredSize.x, data->DesiredSize.y };
}

CK_DLL_MFUN(ui_size_callback_data_set_desired_size)
{
    cimgui::ImGuiSizeCallbackData* data
      = (cimgui::ImGuiSizeCallbackData*)OBJ_MEMBER_UINT(
        SELF, ui_size_callback_data_ptr_offset);
    t_CKVEC2 desired_size = GET_NEXT_VEC2(ARGS);
    data->DesiredSize     = { (float)desired_size.x, (float)desired_size.y };
}

// ============================================================================
// Main Widgets
// ============================================================================

CK_DLL_SFUN(ui_button)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));
    bool ret          = cimgui::ImGui_Button(label);
    RETURN->v_int     = ret;
}

CK_DLL_SFUN(ui_checkbox)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));

    Chuck_Object* obj = GET_NEXT_OBJECT(ARGS);
    bool* b           = (bool*)OBJ_MEMBER_UINT(obj, ui_bool_ptr_offset);

    bool ret      = cimgui::ImGui_Checkbox(label, b);
    RETURN->v_int = ret;
}

// ============================================================================
// Styles
// ============================================================================
CK_DLL_SFUN(ui_styleColorsDark)
{
    cimgui::ImGui_StyleColorsDark(NULL);
}

CK_DLL_SFUN(ui_styleColorsLight)
{
    cimgui::ImGui_StyleColorsLight(NULL);
}

CK_DLL_SFUN(ui_styleColorsClassic)
{
    cimgui::ImGui_StyleColorsClassic(NULL);
}