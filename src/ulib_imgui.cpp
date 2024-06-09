#include <chuck/chugin.h>

// Wrap this in a namespace to keep it separate from the C++ API
namespace cimgui
{
#include <cimgui/cimgui.h>
}

// #ifdef __GNUC__
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wpedantic"
// #define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
// #endif

// clang-format off
// #include <imgui/imgui.h>
// #include <imgui/imgui_internal.h>
// #include <cimgui.h>
// clang-format on

static t_CKUINT ui_bool_ptr_offset = 0;

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
    bool ret      = cimgui::ImGui_Begin("Window", NULL, 0);
    RETURN->v_int = ret;
}

CK_DLL_SFUN(ui_end)
{
    cimgui::ImGui_End();
}

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
void ulib_imgui_query(Chuck_DL_Query* QUERY)
{
    QUERY->begin_class(QUERY, "UI_Bool", "Object");
    ui_bool_ptr_offset = QUERY->add_mvar(QUERY, "int", "@ui_bool_ptr", false);

    QUERY->add_ctor(QUERY, ui_bool_ctor);
    QUERY->add_dtor(QUERY, ui_bool_dtor);
    QUERY->add_mfun(QUERY, ui_bool_get_value, "int", "val");
    QUERY->add_mfun(QUERY, ui_bool_set_value, "int", "val");
    QUERY->add_arg(QUERY, "int", "val");

    QUERY->end_class(QUERY); // UI_Bool

    QUERY->begin_class(QUERY, "UI", "Object");

    // static vars

    // static fns
    QUERY->add_sfun(QUERY, ui_begin, "int", "begin");
    QUERY->add_sfun(QUERY, ui_end, "void", "end");

    QUERY->add_sfun(QUERY, ui_button, "int", "button");
    QUERY->add_arg(QUERY, "string", "label");

    QUERY->add_sfun(QUERY, ui_checkbox, "int", "checkbox");
    QUERY->add_arg(QUERY, "string", "label");
    QUERY->add_arg(QUERY, "UI_Bool", "bool");

    QUERY->end_class(QUERY); // UI
}