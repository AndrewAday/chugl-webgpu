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

// Content region
CK_DLL_SFUN(
  ui_GetContentRegionAvail); // == GetContentRegionMax() - GetCursorPos()
CK_DLL_SFUN(
  ui_GetContentRegionMax); // current content boundaries (typically window
                           // boundaries including scrolling, or current column
                           // boundaries), in windows coordinates
CK_DLL_SFUN(
  ui_GetWindowContentRegionMin); // content boundaries min for the full window
                                 // (roughly (0,0)-Scroll), in window
                                 // coordinates
CK_DLL_SFUN(
  ui_GetWindowContentRegionMax); // content boundaries max for the full window
                                 // (roughly (0,0)+Size-Scroll) where Size can
                                 // be overridden with
                                 // SetNextWindowContentSize(), in window
                                 // coordinates

// Windows Scrolling
// - Any change of Scroll will be applied at the beginning of next frame in the
// first call to Begin().
// - You may instead use SetNextWindowScroll() prior to calling Begin() to avoid
// this delay, as an alternative to using SetScrollX()/SetScrollY().
CK_DLL_SFUN(ui_GetScrollX);
CK_DLL_SFUN(ui_GetScrollY);
CK_DLL_SFUN(ui_SetScrollX);
CK_DLL_SFUN(ui_SetScrollY);
CK_DLL_SFUN(ui_GetScrollMaxX);
CK_DLL_SFUN(ui_GetScrollMaxY);
CK_DLL_SFUN(ui_SetScrollHereX);
CK_DLL_SFUN(ui_SetScrollHereY);
CK_DLL_SFUN(ui_SetScrollFromPosX);
CK_DLL_SFUN(ui_SetScrollFromPosY);

// Parameters stacks (shared)
// ignoring fonts for now (complicated)
// CIMGUI_API void ImGui_PushFont(ImFont* font);                             //
// use NULL as a shortcut to push default font CIMGUI_API void
// ImGui_PopFont(void);
// CK_DLL_SFUN(ui_PushStyleColor);  // ignoring in favor of vec4 version
CK_DLL_SFUN(ui_PushStyleColorImVec4);
CK_DLL_SFUN(ui_PopStyleColor);
CK_DLL_SFUN(ui_PopStyleColorEx);
CK_DLL_SFUN(ui_PushStyleVar);
CK_DLL_SFUN(ui_PushStyleVarImVec2);
CK_DLL_SFUN(ui_PopStyleVar);
CK_DLL_SFUN(ui_PopStyleVarEx);
CK_DLL_SFUN(ui_PushTabStop);
CK_DLL_SFUN(ui_PopTabStop);
CK_DLL_SFUN(ui_PushButtonRepeat);
CK_DLL_SFUN(ui_PopButtonRepeat);

// Style read access
// - Use the ShowStyleEditor() function to interactively see/edit the colors.
// CK_DLL_SFUN(ui_GetFont); // ignoring fonts for now (complicated)
CK_DLL_SFUN(ui_GetFontSize);
CK_DLL_SFUN(ui_GetFontTexUvWhitePixel);
CK_DLL_SFUN(ui_GetColorU32);
CK_DLL_SFUN(ui_GetColorU32Ex);
CK_DLL_SFUN(ui_GetColorU32ImVec4);
// CK_DLL_SFUN(ui_GetColorU32ImU32);  // redundant with GetColorU32Ex
// CK_DLL_SFUN(ui_GetColorU32ImU32Ex); // redundant with GetColorU32Ex
CK_DLL_SFUN(ui_GetStyleColorVec4);

// Layout cursor positioning
// - By "cursor" we mean the current output position.
// - The typical widget behavior is to output themselves at the current cursor
// position, then move the cursor one line down.
// - You can call SameLine() between widgets to undo the last carriage return
// and output at the right of the preceding widget.
// - Attention! We currently have inconsistencies between window-local and
// absolute positions we will aim to fix with future API:
//    - Absolute coordinate:        GetCursorScreenPos(), SetCursorScreenPos(),
//    all ImDrawList:: functions. -> this is the preferred way forward.
//    - Window-local coordinates:   SameLine(), GetCursorPos(), SetCursorPos(),
//    GetCursorStartPos(), GetContentRegionMax(), GetWindowContentRegion*(),
//    PushTextWrapPos()
// - GetCursorScreenPos() = GetCursorPos() + GetWindowPos(). GetWindowPos() is
// almost only ever useful to convert from window-local to absolute coordinates.
CK_DLL_SFUN(ui_GetCursorScreenPos);
CK_DLL_SFUN(ui_SetCursorScreenPos);
CK_DLL_SFUN(ui_GetCursorPos);
CK_DLL_SFUN(ui_GetCursorPosX);
CK_DLL_SFUN(ui_GetCursorPosY);
CK_DLL_SFUN(ui_SetCursorPos);
CK_DLL_SFUN(ui_SetCursorPosX);
CK_DLL_SFUN(ui_SetCursorPosY);
CK_DLL_SFUN(ui_GetCursorStartPos);

// Other layout functions
CK_DLL_SFUN(ui_Separator);
CK_DLL_SFUN(ui_SameLine);
CK_DLL_SFUN(ui_SameLineEx);
CK_DLL_SFUN(ui_NewLine);
CK_DLL_SFUN(ui_Spacing);
CK_DLL_SFUN(ui_Dummy);
CK_DLL_SFUN(ui_Indent);
CK_DLL_SFUN(ui_IndentEx);
CK_DLL_SFUN(ui_Unindent);
CK_DLL_SFUN(ui_UnindentEx);
CK_DLL_SFUN(ui_BeginGroup);
CK_DLL_SFUN(ui_EndGroup);
CK_DLL_SFUN(ui_AlignTextToFramePadding);
CK_DLL_SFUN(ui_GetTextLineHeight);
CK_DLL_SFUN(ui_GetTextLineHeightWithSpacing);
CK_DLL_SFUN(ui_GetFrameHeight);
CK_DLL_SFUN(ui_GetFrameHeightWithSpacing);

// ID stack/scopes
// Read the FAQ (docs/FAQ.md or http://dearimgui.com/faq) for more details about
// how ID are handled in dear imgui.
// - Those questions are answered and impacted by understanding of the ID stack
// system:
//   - "Q: Why is my widget not reacting when I click on it?"
//   - "Q: How can I have widgets with an empty label?"
//   - "Q: How can I have multiple widgets with the same label?"
// - Short version: ID are hashes of the entire ID stack. If you are creating
// widgets in a loop you most likely
//   want to push a unique identifier (e.g. object pointer, loop index) to
//   uniquely differentiate them.
// - You can also use the "Label##foobar" syntax within widget label to
// distinguish them from each others.
// - In this header file we use the "label"/"name" terminology to denote a
// string that will be displayed + used as an ID,
//   whereas "str_id" denote a string that is only used as an ID and not
//   normally displayed.
CK_DLL_SFUN(ui_PushID);
CK_DLL_SFUN(ui_PushIDStr);
// CK_DLL_SFUN(ui_PushIDPtr); // ignore. no pointers in chuck
CK_DLL_SFUN(ui_PushIDInt);
CK_DLL_SFUN(ui_PopID);
CK_DLL_SFUN(ui_GetID);
CK_DLL_SFUN(ui_GetIDStr);
// CK_DLL_SFUN(ui_GetIDPtr);  // ignore. no pointers in chuck

// Widgets: Text (ignoring formated / va_args)
CK_DLL_SFUN(ui_TextUnformatted);
CK_DLL_SFUN(ui_TextUnformattedEx);
// CK_DLL_SFUN(ui_Text);
// CK_DLL_SFUN(ui_TextV);
// CK_DLL_SFUN(ui_TextColored);
CK_DLL_SFUN(ui_TextColoredUnformatted);
// CK_DLL_SFUN(ui_TextColoredV);
// CK_DLL_SFUN(ui_TextDisabled);
CK_DLL_SFUN(ui_TextDisabledUnformatted);
// CK_DLL_SFUN(ui_TextDisabledV);
// CK_DLL_SFUN(ui_TextWrapped);
CK_DLL_SFUN(ui_TextWrappedUnformatted);
// CK_DLL_SFUN(ui_TextWrappedV);
// CK_DLL_SFUN(ui_LabelText);
CK_DLL_SFUN(ui_LabelTextUnformatted);
// CK_DLL_SFUN(ui_LabelTextV);
// CK_DLL_SFUN(ui_BulletText);
CK_DLL_SFUN(ui_BulletTextUnformatted);
// CK_DLL_SFUN(ui_BulletTextV);
CK_DLL_SFUN(ui_SeparatorText);

// Widgets: Main
// - Most widgets return true when the value has been changed or when
// pressed/selected
// - You may also use one of the many IsItemXXX functions (e.g. IsItemActive,
// IsItemHovered, etc.) to query widget state.
CK_DLL_SFUN(ui_Button);
CK_DLL_SFUN(ui_ButtonEx);
CK_DLL_SFUN(ui_SmallButton);
CK_DLL_SFUN(ui_InvisibleButton);
CK_DLL_SFUN(ui_ArrowButton);
CK_DLL_SFUN(ui_Checkbox);
CK_DLL_SFUN(ui_CheckboxFlagsIntPtr);
// CK_DLL_SFUN(ui_CheckboxFlagsUintPtr); // redundant
CK_DLL_SFUN(ui_RadioButton);
CK_DLL_SFUN(ui_RadioButtonIntPtr);
CK_DLL_SFUN(ui_ProgressBar);
CK_DLL_SFUN(ui_Bullet);

// Widgets: Images (TODO - figure out how to handle ImTextureID)
// - Read about ImTextureID here:
// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
// - 'uv0' and 'uv1' are texture coordinates. Read about them from the same link
// above.
// - Note that Image() may add +2.0f to provided size if a border is visible,
// ImageButton() adds style.FramePadding*2.0f to provided size. CIMGUI_API void
// ImGui_Image(ImTextureID user_texture_id, ImVec2 image_size); // Implied uv0 =
// ImVec2(0, 0), uv1 = ImVec2(1, 1), tint_col = ImVec4(1, 1, 1, 1), border_col =
// ImVec4(0, 0, 0, 0) CIMGUI_API void ImGui_ImageEx(ImTextureID user_texture_id,
// ImVec2 image_size, ImVec2 uv0 /* = ImVec2(0, 0) */, ImVec2 uv1 /* = ImVec2(1,
// 1) */, ImVec4 tint_col /* = ImVec4(1, 1, 1, 1) */, ImVec4 border_col /* =
// ImVec4(0, 0, 0, 0) */); CIMGUI_API bool ImGui_ImageButton(const char* str_id,
// ImTextureID user_texture_id, ImVec2 image_size);  // Implied uv0 = ImVec2(0,
// 0), uv1 = ImVec2(1, 1), bg_col = ImVec4(0, 0, 0, 0), tint_col = ImVec4(1, 1,
// 1, 1) CIMGUI_API bool ImGui_ImageButtonEx(const char* str_id, ImTextureID
// user_texture_id, ImVec2 image_size, ImVec2 uv0 /* = ImVec2(0, 0) */, ImVec2
// uv1 /* = ImVec2(1, 1) */, ImVec4 bg_col /* = ImVec4(0, 0, 0, 0) */, ImVec4
// tint_col /* = ImVec4(1, 1, 1, 1) */);

// Widgets: Combo Box (Dropdown)
// - The BeginCombo()/EndCombo() api allows you to manage your contents and
// selection state however you want it, by creating e.g. Selectable() items.
// - The old Combo() api are helpers over BeginCombo()/EndCombo() which are kept
// available for convenience purpose. This is analogous to how ListBox are
// created.
CK_DLL_SFUN(ui_BeginCombo);
CK_DLL_SFUN(ui_EndCombo);
CK_DLL_SFUN(ui_ComboChar);
CK_DLL_SFUN(ui_ComboCharEx);
CK_DLL_SFUN(ui_Combo);   // TODO: doesn't work with chuck strings
CK_DLL_SFUN(ui_ComboEx); // TODO: doesn't work with chuck strings
// ignoring callback versions. seems redundant and more complicated
// CK_DLL_SFUN(ui_ComboCallback);
// CK_DLL_SFUN(ui_ComboCallbackEx);

// Widgets: Drag Sliders
CK_DLL_SFUN(ui_DragFloat);
CK_DLL_SFUN(ui_DragFloatEx);
CK_DLL_SFUN(ui_DragFloat2);
CK_DLL_SFUN(ui_DragFloat2Ex);
CK_DLL_SFUN(ui_DragFloat3);
CK_DLL_SFUN(ui_DragFloat3Ex);
CK_DLL_SFUN(ui_DragFloat4);
CK_DLL_SFUN(ui_DragFloat4Ex);
CK_DLL_SFUN(ui_DragFloatRange2);
CK_DLL_SFUN(ui_DragFloatRange2Ex);
CK_DLL_SFUN(ui_DragInt);
CK_DLL_SFUN(ui_DragIntEx);
CK_DLL_SFUN(ui_DragInt2);
CK_DLL_SFUN(ui_DragInt2Ex);
CK_DLL_SFUN(ui_DragInt3);
CK_DLL_SFUN(ui_DragInt3Ex);
CK_DLL_SFUN(ui_DragInt4);
CK_DLL_SFUN(ui_DragInt4Ex);
CK_DLL_SFUN(ui_DragIntRange2);
CK_DLL_SFUN(ui_DragIntRange2Ex);
// CK_DLL_SFUN(ui_DragScalar); // ignore, chuck doesn't have scalar types like
// U8, S16, etc. CK_DLL_SFUN(ui_DragScalarEx); // ignore, same reason
CK_DLL_SFUN(ui_DragScalarN_CKINT);
CK_DLL_SFUN(ui_DragScalarNEx_CKINT);
CK_DLL_SFUN(ui_DragScalarN_CKFLOAT);
CK_DLL_SFUN(ui_DragScalarNEx_CKFLOAT);

// Widgets: Regular Sliders
CK_DLL_SFUN(ui_SliderFloat);
CK_DLL_SFUN(ui_SliderFloatEx);
CK_DLL_SFUN(ui_SliderAngle);
CK_DLL_SFUN(ui_SliderAngleEx);
CK_DLL_SFUN(ui_SliderInt);
CK_DLL_SFUN(ui_SliderIntEx);
CK_DLL_SFUN(ui_SliderScalarN_CKINT);
CK_DLL_SFUN(ui_SliderScalarNEx_CKINT);
CK_DLL_SFUN(ui_SliderScalarN_CKFLOAT);
CK_DLL_SFUN(ui_SliderScalarNEx_CKFLOAT);
CK_DLL_SFUN(ui_VSliderFloat);
CK_DLL_SFUN(ui_VSliderFloatEx);
CK_DLL_SFUN(ui_VSliderInt);
CK_DLL_SFUN(ui_VSliderIntEx);

// Styles
CK_DLL_SFUN(ui_styleColorsDark);
CK_DLL_SFUN(ui_styleColorsLight);
CK_DLL_SFUN(ui_styleColorsClassic);
CK_DLL_SFUN(ui_showStyleEditor);

// Callbacks -----------------------------------------------------------------

// base callback
CK_DLL_CTOR(ui_callback_ctor);
CK_DLL_DTOR(ui_callback_dtor);

// ImGuiSizeCallback
CK_DLL_MFUN(ui_size_callback);
static t_CKINT ui_size_callback_vt_offset = -1;
// ImGuiSizeCallbackData
static t_CKUINT ui_size_callback_data_ptr_offset = 0;
CK_DLL_MFUN(ui_size_callback_data_get_pos);
CK_DLL_MFUN(ui_size_callback_data_get_current_size);
CK_DLL_MFUN(ui_size_callback_data_get_desired_size);
CK_DLL_MFUN(ui_size_callback_data_set_desired_size);

// ComboCallback
CK_DLL_MFUN(ui_combo_callback);
static t_CKINT ui_combo_callback_vt_offset = -1;

// UI_Bool
static t_CKUINT ui_bool_ptr_offset = 0;
CK_DLL_CTOR(ui_bool_ctor);
CK_DLL_DTOR(ui_bool_dtor);
CK_DLL_MFUN(ui_bool_get_value);
CK_DLL_MFUN(ui_bool_set_value);

// UI_Int
static t_CKUINT ui_int_ptr_offset = 0;
CK_DLL_CTOR(ui_int_ctor);
CK_DLL_DTOR(ui_int_dtor);
CK_DLL_MFUN(ui_int_get_value);
CK_DLL_MFUN(ui_int_set_value);

// UI_Int2
static t_CKUINT ui_int2_ptr_offset = 0;
CK_DLL_CTOR(ui_int2_ctor);
CK_DLL_DTOR(ui_int2_dtor);
CK_DLL_MFUN(ui_int2_get_x);
CK_DLL_MFUN(ui_int2_get_y);
CK_DLL_MFUN(ui_int2_set);

// UI_Int3
static t_CKUINT ui_int3_ptr_offset = 0;
CK_DLL_CTOR(ui_int3_ctor);
CK_DLL_DTOR(ui_int3_dtor);
CK_DLL_MFUN(ui_int3_get_x);
CK_DLL_MFUN(ui_int3_get_y);
CK_DLL_MFUN(ui_int3_get_z);
CK_DLL_MFUN(ui_int3_set);

// UI_Int4
static t_CKUINT ui_int4_ptr_offset = 0;
CK_DLL_CTOR(ui_int4_ctor);
CK_DLL_DTOR(ui_int4_dtor);
CK_DLL_MFUN(ui_int4_get_x);
CK_DLL_MFUN(ui_int4_get_y);
CK_DLL_MFUN(ui_int4_get_z);
CK_DLL_MFUN(ui_int4_get_w);
CK_DLL_MFUN(ui_int4_set);

// UI_Float
static t_CKUINT ui_float_ptr_offset = 0;
CK_DLL_CTOR(ui_float_ctor);
CK_DLL_DTOR(ui_float_dtor);
CK_DLL_MFUN(ui_float_get_value);
CK_DLL_MFUN(ui_float_set_value);

// UI_Float2
static t_CKUINT ui_float2_ptr_offset = 0;
CK_DLL_CTOR(ui_float2_ctor);
CK_DLL_DTOR(ui_float2_dtor);
CK_DLL_MFUN(ui_float2_get_value);
CK_DLL_MFUN(ui_float2_set_value);

// UI_Float3
static t_CKUINT ui_float3_ptr_offset = 0;
CK_DLL_CTOR(ui_float3_ctor);
CK_DLL_DTOR(ui_float3_dtor);
CK_DLL_MFUN(ui_float3_get_value);
CK_DLL_MFUN(ui_float3_set_value);

// UI_Float4
static t_CKUINT ui_float4_ptr_offset = 0;
CK_DLL_CTOR(ui_float4_ctor);
CK_DLL_DTOR(ui_float4_dtor);
CK_DLL_MFUN(ui_float4_get_value);
CK_DLL_MFUN(ui_float4_set_value);

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

    // UI_Int ---------------------------------------------------------------
    QUERY->begin_class(QUERY, "UI_Int", "Object");
    ui_int_ptr_offset = QUERY->add_mvar(QUERY, "int", "@ui_int_ptr", false);

    QUERY->add_ctor(QUERY, ui_int_ctor);
    QUERY->add_dtor(QUERY, ui_int_dtor);
    QUERY->add_mfun(QUERY, ui_int_get_value, "int", "val");
    QUERY->add_mfun(QUERY, ui_int_set_value, "int", "val");
    QUERY->add_arg(QUERY, "int", "val");
    QUERY->end_class(QUERY); // UI_Int

    // UI_Int2 ---------------------------------------------------------------
    BEGIN_CLASS("UI_Int2", "Object");
    ui_int2_ptr_offset = MVAR("int", "@ui_int2_ptr", false);
    CTOR(ui_int2_ctor);
    DTOR(ui_int2_dtor);
    MFUN(ui_int2_get_x, "int", "x");
    MFUN(ui_int2_get_y, "int", "y");
    MFUN(ui_int2_set, "void", "val");
    ARG("int", "x");
    ARG("int", "y");
    END_CLASS(); // UI_Int2

    // UI_Int3 ---------------------------------------------------------------
    BEGIN_CLASS("UI_Int3", "Object");
    ui_int3_ptr_offset = MVAR("int", "@ui_int3_ptr", false);
    CTOR(ui_int3_ctor);
    DTOR(ui_int3_dtor);
    MFUN(ui_int3_get_x, "int", "x");
    MFUN(ui_int3_get_y, "int", "y");
    MFUN(ui_int3_get_z, "int", "z");
    MFUN(ui_int3_set, "void", "val");
    ARG("int", "x");
    ARG("int", "y");
    ARG("int", "z");
    END_CLASS(); // UI_Int3

    // UI_Int4 ---------------------------------------------------------------
    BEGIN_CLASS("UI_Int4", "Object");
    ui_int4_ptr_offset = MVAR("int", "@ui_int4_ptr", false);
    CTOR(ui_int4_ctor);
    DTOR(ui_int4_dtor);
    MFUN(ui_int4_get_x, "int", "x");
    MFUN(ui_int4_get_y, "int", "y");
    MFUN(ui_int4_get_z, "int", "z");
    MFUN(ui_int4_get_w, "int", "w");
    MFUN(ui_int4_set, "void", "val");
    ARG("int", "x");
    ARG("int", "y");
    ARG("int", "z");
    ARG("int", "w");
    END_CLASS(); // UI_Int4

    // UI_Float ---------------------------------------------------------------
    BEGIN_CLASS("UI_Float", "Object");
    ui_float_ptr_offset = MVAR("int", "@ui_float_ptr", false);
    CTOR(ui_float_ctor);
    DTOR(ui_float_dtor);
    MFUN(ui_float_get_value, "float", "val");
    MFUN(ui_float_set_value, "float", "val");
    ARG("float", "val");
    END_CLASS(); // UI_Float

    // UI_Float2 ---------------------------------------------------------------
    BEGIN_CLASS("UI_Float2", "Object");
    ui_float2_ptr_offset = MVAR("int", "@ui_float2_ptr", false);
    CTOR(ui_float2_ctor);
    DTOR(ui_float2_dtor);
    MFUN(ui_float2_get_value, "vec2", "val");
    MFUN(ui_float2_set_value, "vec2", "val");
    ARG("vec2", "val");
    END_CLASS(); // UI_Float2

    // UI_Float3 ---------------------------------------------------------------
    BEGIN_CLASS("UI_Float3", "Object");
    ui_float3_ptr_offset = MVAR("int", "@ui_float3_ptr", false);
    CTOR(ui_float3_ctor);
    DTOR(ui_float3_dtor);
    MFUN(ui_float3_get_value, "vec3", "val");
    MFUN(ui_float3_set_value, "vec3", "val");
    ARG("vec3", "val");
    END_CLASS(); // UI_Float3

    // UI_Float4 ---------------------------------------------------------------
    BEGIN_CLASS("UI_Float4", "Object");
    ui_float4_ptr_offset = MVAR("int", "@ui_float4_ptr", false);
    CTOR(ui_float4_ctor);
    DTOR(ui_float4_dtor);
    MFUN(ui_float4_get_value, "vec4", "val");
    MFUN(ui_float4_set_value, "vec4", "val");
    ARG("vec4", "val");
    END_CLASS(); // UI_Float4

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

    QUERY->begin_class(QUERY, "UI_Color", "Object");
    QUERY->doc_class(QUERY,
                     "Enumeration for PushStyleColor() / PopStyleColor().\n A "
                     "color identifier for styling");
    static t_CKINT ImGuiCol_Text = 0;
    QUERY->add_svar(QUERY, "int", "Text", true, &ImGuiCol_Text);
    static t_CKINT ImGuiCol_TextDisabled = 1;
    QUERY->add_svar(QUERY, "int", "TextDisabled", true, &ImGuiCol_TextDisabled);
    static t_CKINT ImGuiCol_WindowBg = 2;
    QUERY->add_svar(QUERY, "int", "WindowBg", true, &ImGuiCol_WindowBg);
    QUERY->doc_var(QUERY, "Background of normal windows");
    static t_CKINT ImGuiCol_ChildBg = 3;
    QUERY->add_svar(QUERY, "int", "ChildBg", true, &ImGuiCol_ChildBg);
    QUERY->doc_var(QUERY, "Background of child windows");
    static t_CKINT ImGuiCol_PopupBg = 4;
    QUERY->add_svar(QUERY, "int", "PopupBg", true, &ImGuiCol_PopupBg);
    QUERY->doc_var(QUERY, "Background of popups, menus, tooltips windows");
    static t_CKINT ImGuiCol_Border = 5;
    QUERY->add_svar(QUERY, "int", "Border", true, &ImGuiCol_Border);
    static t_CKINT ImGuiCol_BorderShadow = 6;
    QUERY->add_svar(QUERY, "int", "BorderShadow", true, &ImGuiCol_BorderShadow);
    static t_CKINT ImGuiCol_FrameBg = 7;
    QUERY->add_svar(QUERY, "int", "FrameBg", true, &ImGuiCol_FrameBg);
    QUERY->doc_var(
      QUERY, "Background of checkbox, radio button, plot, slider, text input");
    static t_CKINT ImGuiCol_FrameBgHovered = 8;
    QUERY->add_svar(QUERY, "int", "FrameBgHovered", true,
                    &ImGuiCol_FrameBgHovered);
    static t_CKINT ImGuiCol_FrameBgActive = 9;
    QUERY->add_svar(QUERY, "int", "FrameBgActive", true,
                    &ImGuiCol_FrameBgActive);
    static t_CKINT ImGuiCol_TitleBg = 10;
    QUERY->add_svar(QUERY, "int", "TitleBg", true, &ImGuiCol_TitleBg);
    QUERY->doc_var(QUERY, "Title bar");
    static t_CKINT ImGuiCol_TitleBgActive = 11;
    QUERY->add_svar(QUERY, "int", "TitleBgActive", true,
                    &ImGuiCol_TitleBgActive);
    QUERY->doc_var(QUERY, "Title bar when focused");
    static t_CKINT ImGuiCol_TitleBgCollapsed = 12;
    QUERY->add_svar(QUERY, "int", "TitleBgCollapsed", true,
                    &ImGuiCol_TitleBgCollapsed);
    QUERY->doc_var(QUERY, "Title bar when collapsed");
    static t_CKINT ImGuiCol_MenuBarBg = 13;
    QUERY->add_svar(QUERY, "int", "MenuBarBg", true, &ImGuiCol_MenuBarBg);
    static t_CKINT ImGuiCol_ScrollbarBg = 14;
    QUERY->add_svar(QUERY, "int", "ScrollbarBg", true, &ImGuiCol_ScrollbarBg);
    static t_CKINT ImGuiCol_ScrollbarGrab = 15;
    QUERY->add_svar(QUERY, "int", "ScrollbarGrab", true,
                    &ImGuiCol_ScrollbarGrab);
    static t_CKINT ImGuiCol_ScrollbarGrabHovered = 16;
    QUERY->add_svar(QUERY, "int", "ScrollbarGrabHovered", true,
                    &ImGuiCol_ScrollbarGrabHovered);
    static t_CKINT ImGuiCol_ScrollbarGrabActive = 17;
    QUERY->add_svar(QUERY, "int", "ScrollbarGrabActive", true,
                    &ImGuiCol_ScrollbarGrabActive);
    static t_CKINT ImGuiCol_CheckMark = 18;
    QUERY->add_svar(QUERY, "int", "CheckMark", true, &ImGuiCol_CheckMark);
    QUERY->doc_var(QUERY, "Checkbox tick and RadioButton circle");
    static t_CKINT ImGuiCol_SliderGrab = 19;
    QUERY->add_svar(QUERY, "int", "SliderGrab", true, &ImGuiCol_SliderGrab);
    static t_CKINT ImGuiCol_SliderGrabActive = 20;
    QUERY->add_svar(QUERY, "int", "SliderGrabActive", true,
                    &ImGuiCol_SliderGrabActive);
    static t_CKINT ImGuiCol_Button = 21;
    QUERY->add_svar(QUERY, "int", "Button", true, &ImGuiCol_Button);
    static t_CKINT ImGuiCol_ButtonHovered = 22;
    QUERY->add_svar(QUERY, "int", "ButtonHovered", true,
                    &ImGuiCol_ButtonHovered);
    static t_CKINT ImGuiCol_ButtonActive = 23;
    QUERY->add_svar(QUERY, "int", "ButtonActive", true, &ImGuiCol_ButtonActive);
    static t_CKINT ImGuiCol_Header = 24;
    QUERY->add_svar(QUERY, "int", "Header", true, &ImGuiCol_Header);
    QUERY->doc_var(QUERY,
                   "Header* colors are used for CollapsingHeader, TreeNode, "
                   "Selectable, MenuItem");
    static t_CKINT ImGuiCol_HeaderHovered = 25;
    QUERY->add_svar(QUERY, "int", "HeaderHovered", true,
                    &ImGuiCol_HeaderHovered);
    static t_CKINT ImGuiCol_HeaderActive = 26;
    QUERY->add_svar(QUERY, "int", "HeaderActive", true, &ImGuiCol_HeaderActive);
    static t_CKINT ImGuiCol_Separator = 27;
    QUERY->add_svar(QUERY, "int", "Separator", true, &ImGuiCol_Separator);
    static t_CKINT ImGuiCol_SeparatorHovered = 28;
    QUERY->add_svar(QUERY, "int", "SeparatorHovered", true,
                    &ImGuiCol_SeparatorHovered);
    static t_CKINT ImGuiCol_SeparatorActive = 29;
    QUERY->add_svar(QUERY, "int", "SeparatorActive", true,
                    &ImGuiCol_SeparatorActive);
    static t_CKINT ImGuiCol_ResizeGrip = 30;
    QUERY->add_svar(QUERY, "int", "ResizeGrip", true, &ImGuiCol_ResizeGrip);
    QUERY->doc_var(
      QUERY, "Resize grip in lower-right and lower-left corners of windows.");
    static t_CKINT ImGuiCol_ResizeGripHovered = 31;
    QUERY->add_svar(QUERY, "int", "ResizeGripHovered", true,
                    &ImGuiCol_ResizeGripHovered);
    static t_CKINT ImGuiCol_ResizeGripActive = 32;
    QUERY->add_svar(QUERY, "int", "ResizeGripActive", true,
                    &ImGuiCol_ResizeGripActive);
    static t_CKINT ImGuiCol_Tab = 33;
    QUERY->add_svar(QUERY, "int", "Tab", true, &ImGuiCol_Tab);
    QUERY->doc_var(QUERY, "TabItem in a TabBar");
    static t_CKINT ImGuiCol_TabHovered = 34;
    QUERY->add_svar(QUERY, "int", "TabHovered", true, &ImGuiCol_TabHovered);
    static t_CKINT ImGuiCol_TabActive = 35;
    QUERY->add_svar(QUERY, "int", "TabActive", true, &ImGuiCol_TabActive);
    static t_CKINT ImGuiCol_TabUnfocused = 36;
    QUERY->add_svar(QUERY, "int", "TabUnfocused", true, &ImGuiCol_TabUnfocused);
    static t_CKINT ImGuiCol_TabUnfocusedActive = 37;
    QUERY->add_svar(QUERY, "int", "TabUnfocusedActive", true,
                    &ImGuiCol_TabUnfocusedActive);
    static t_CKINT ImGuiCol_DockingPreview = 38;
    QUERY->add_svar(QUERY, "int", "DockingPreview", true,
                    &ImGuiCol_DockingPreview);
    QUERY->doc_var(QUERY,
                   "Preview overlay color when about to docking something");
    static t_CKINT ImGuiCol_DockingEmptyBg = 39;
    QUERY->add_svar(QUERY, "int", "DockingEmptyBg", true,
                    &ImGuiCol_DockingEmptyBg);
    QUERY->doc_var(QUERY,
                   "Background color for empty node (e.g. CentralNode with no "
                   "window docked into it)");
    static t_CKINT ImGuiCol_PlotLines = 40;
    QUERY->add_svar(QUERY, "int", "PlotLines", true, &ImGuiCol_PlotLines);
    static t_CKINT ImGuiCol_PlotLinesHovered = 41;
    QUERY->add_svar(QUERY, "int", "PlotLinesHovered", true,
                    &ImGuiCol_PlotLinesHovered);
    static t_CKINT ImGuiCol_PlotHistogram = 42;
    QUERY->add_svar(QUERY, "int", "PlotHistogram", true,
                    &ImGuiCol_PlotHistogram);
    static t_CKINT ImGuiCol_PlotHistogramHovered = 43;
    QUERY->add_svar(QUERY, "int", "PlotHistogramHovered", true,
                    &ImGuiCol_PlotHistogramHovered);
    static t_CKINT ImGuiCol_TableHeaderBg = 44;
    QUERY->add_svar(QUERY, "int", "TableHeaderBg", true,
                    &ImGuiCol_TableHeaderBg);
    QUERY->doc_var(QUERY, "Table header background");
    static t_CKINT ImGuiCol_TableBorderStrong = 45;
    QUERY->add_svar(QUERY, "int", "TableBorderStrong", true,
                    &ImGuiCol_TableBorderStrong);
    QUERY->doc_var(
      QUERY, "Table outer and header borders (prefer using Alpha=1.0 here)");
    static t_CKINT ImGuiCol_TableBorderLight = 46;
    QUERY->add_svar(QUERY, "int", "TableBorderLight", true,
                    &ImGuiCol_TableBorderLight);
    QUERY->doc_var(QUERY, "Table inner borders (prefer using Alpha=1.0 here)");
    static t_CKINT ImGuiCol_TableRowBg = 47;
    QUERY->add_svar(QUERY, "int", "TableRowBg", true, &ImGuiCol_TableRowBg);
    QUERY->doc_var(QUERY, "Table row background (even rows)");
    static t_CKINT ImGuiCol_TableRowBgAlt = 48;
    QUERY->add_svar(QUERY, "int", "TableRowBgAlt", true,
                    &ImGuiCol_TableRowBgAlt);
    QUERY->doc_var(QUERY, "Table row background (odd rows)");
    static t_CKINT ImGuiCol_TextSelectedBg = 49;
    QUERY->add_svar(QUERY, "int", "TextSelectedBg", true,
                    &ImGuiCol_TextSelectedBg);
    static t_CKINT ImGuiCol_DragDropTarget = 50;
    QUERY->add_svar(QUERY, "int", "DragDropTarget", true,
                    &ImGuiCol_DragDropTarget);
    QUERY->doc_var(QUERY, "Rectangle highlighting a drop target");
    static t_CKINT ImGuiCol_NavHighlight = 51;
    QUERY->add_svar(QUERY, "int", "NavHighlight", true, &ImGuiCol_NavHighlight);
    QUERY->doc_var(QUERY, "Gamepad/keyboard: current highlighted item");
    static t_CKINT ImGuiCol_NavWindowingHighlight = 52;
    QUERY->add_svar(QUERY, "int", "NavWindowingHighlight", true,
                    &ImGuiCol_NavWindowingHighlight);
    QUERY->doc_var(QUERY, "Highlight window when using CTRL+TAB");
    static t_CKINT ImGuiCol_NavWindowingDimBg = 53;
    QUERY->add_svar(QUERY, "int", "NavWindowingDimBg", true,
                    &ImGuiCol_NavWindowingDimBg);
    QUERY->doc_var(QUERY,
                   "Darken/colorize entire screen behind the CTRL+TAB window "
                   "list, when active");
    static t_CKINT ImGuiCol_ModalWindowDimBg = 54;
    QUERY->add_svar(QUERY, "int", "ModalWindowDimBg", true,
                    &ImGuiCol_ModalWindowDimBg);
    QUERY->doc_var(QUERY,
                   "Darken/colorize entire screen behind a modal window, when "
                   "one is active");
    static t_CKINT ImGuiCol_COUNT = 55;
    QUERY->add_svar(QUERY, "int", "COUNT", true, &ImGuiCol_COUNT);
    QUERY->end_class(QUERY);

    QUERY->begin_class(QUERY, "UI_StyleVar", "Object");
    QUERY->doc_class(
      QUERY,
      "Enumeration for PushStyleVar() / PopStyleVar() to temporarily modify "
      "the ImGuiStyle structure..\n- The enum only refers to fields of "
      "ImGuiStyle which makes sense to be pushed/popped inside UI "
      "code..\nDuring initialization or between frames, feel free to just poke "
      "into ImGuiStyle directly..\n- Tip: Use your programming IDE navigation "
      "facilities on the names in the _second column_ below to find the actual "
      "members and their description..\n- In Visual Studio: CTRL+comma "
      "(\"Edit.GoToAll\") can follow symbols inside comments, whereas CTRL+F12 "
      "(\"Edit.GoToImplementation\") cannot..\n- In Visual Studio w/ Visual "
      "Assist installed: ALT+G (\"VAssistX.GoToImplementation\") can also "
      "follow symbols inside comments..\n- In VS Code, CLion, etc.: CTRL+click "
      "can follow symbols inside comments..\n- When changing this enum, you "
      "need to update the associated internal table GStyleVarInfo[] "
      "accordingly. This is where we link enum values to members "
      "offset/type..\n");
    static t_CKINT ImGuiStyleVar_Alpha = 0;
    QUERY->add_svar(QUERY, "int", "Alpha", true, &ImGuiStyleVar_Alpha);
    QUERY->doc_var(QUERY, "float     Alpha");
    static t_CKINT ImGuiStyleVar_DisabledAlpha = 1;
    QUERY->add_svar(QUERY, "int", "DisabledAlpha", true,
                    &ImGuiStyleVar_DisabledAlpha);
    QUERY->doc_var(QUERY, "float     DisabledAlpha");
    static t_CKINT ImGuiStyleVar_WindowPadding = 2;
    QUERY->add_svar(QUERY, "int", "WindowPadding", true,
                    &ImGuiStyleVar_WindowPadding);
    QUERY->doc_var(QUERY, "ImVec2    WindowPadding");
    static t_CKINT ImGuiStyleVar_WindowRounding = 3;
    QUERY->add_svar(QUERY, "int", "WindowRounding", true,
                    &ImGuiStyleVar_WindowRounding);
    QUERY->doc_var(QUERY, "float     WindowRounding");
    static t_CKINT ImGuiStyleVar_WindowBorderSize = 4;
    QUERY->add_svar(QUERY, "int", "WindowBorderSize", true,
                    &ImGuiStyleVar_WindowBorderSize);
    QUERY->doc_var(QUERY, "float     WindowBorderSize");
    static t_CKINT ImGuiStyleVar_WindowMinSize = 5;
    QUERY->add_svar(QUERY, "int", "WindowMinSize", true,
                    &ImGuiStyleVar_WindowMinSize);
    QUERY->doc_var(QUERY, "ImVec2    WindowMinSize");
    static t_CKINT ImGuiStyleVar_WindowTitleAlign = 6;
    QUERY->add_svar(QUERY, "int", "WindowTitleAlign", true,
                    &ImGuiStyleVar_WindowTitleAlign);
    QUERY->doc_var(QUERY, "ImVec2    WindowTitleAlign");
    static t_CKINT ImGuiStyleVar_ChildRounding = 7;
    QUERY->add_svar(QUERY, "int", "ChildRounding", true,
                    &ImGuiStyleVar_ChildRounding);
    QUERY->doc_var(QUERY, "float     ChildRounding");
    static t_CKINT ImGuiStyleVar_ChildBorderSize = 8;
    QUERY->add_svar(QUERY, "int", "ChildBorderSize", true,
                    &ImGuiStyleVar_ChildBorderSize);
    QUERY->doc_var(QUERY, "float     ChildBorderSize");
    static t_CKINT ImGuiStyleVar_PopupRounding = 9;
    QUERY->add_svar(QUERY, "int", "PopupRounding", true,
                    &ImGuiStyleVar_PopupRounding);
    QUERY->doc_var(QUERY, "float     PopupRounding");
    static t_CKINT ImGuiStyleVar_PopupBorderSize = 10;
    QUERY->add_svar(QUERY, "int", "PopupBorderSize", true,
                    &ImGuiStyleVar_PopupBorderSize);
    QUERY->doc_var(QUERY, "float     PopupBorderSize");
    static t_CKINT ImGuiStyleVar_FramePadding = 11;
    QUERY->add_svar(QUERY, "int", "FramePadding", true,
                    &ImGuiStyleVar_FramePadding);
    QUERY->doc_var(QUERY, "ImVec2    FramePadding");
    static t_CKINT ImGuiStyleVar_FrameRounding = 12;
    QUERY->add_svar(QUERY, "int", "FrameRounding", true,
                    &ImGuiStyleVar_FrameRounding);
    QUERY->doc_var(QUERY, "float     FrameRounding");
    static t_CKINT ImGuiStyleVar_FrameBorderSize = 13;
    QUERY->add_svar(QUERY, "int", "FrameBorderSize", true,
                    &ImGuiStyleVar_FrameBorderSize);
    QUERY->doc_var(QUERY, "float     FrameBorderSize");
    static t_CKINT ImGuiStyleVar_ItemSpacing = 14;
    QUERY->add_svar(QUERY, "int", "ItemSpacing", true,
                    &ImGuiStyleVar_ItemSpacing);
    QUERY->doc_var(QUERY, "ImVec2    ItemSpacing");
    static t_CKINT ImGuiStyleVar_ItemInnerSpacing = 15;
    QUERY->add_svar(QUERY, "int", "ItemInnerSpacing", true,
                    &ImGuiStyleVar_ItemInnerSpacing);
    QUERY->doc_var(QUERY, "ImVec2    ItemInnerSpacing");
    static t_CKINT ImGuiStyleVar_IndentSpacing = 16;
    QUERY->add_svar(QUERY, "int", "IndentSpacing", true,
                    &ImGuiStyleVar_IndentSpacing);
    QUERY->doc_var(QUERY, "float     IndentSpacing");
    static t_CKINT ImGuiStyleVar_CellPadding = 17;
    QUERY->add_svar(QUERY, "int", "CellPadding", true,
                    &ImGuiStyleVar_CellPadding);
    QUERY->doc_var(QUERY, "ImVec2    CellPadding");
    static t_CKINT ImGuiStyleVar_ScrollbarSize = 18;
    QUERY->add_svar(QUERY, "int", "ScrollbarSize", true,
                    &ImGuiStyleVar_ScrollbarSize);
    QUERY->doc_var(QUERY, "float     ScrollbarSize");
    static t_CKINT ImGuiStyleVar_ScrollbarRounding = 19;
    QUERY->add_svar(QUERY, "int", "ScrollbarRounding", true,
                    &ImGuiStyleVar_ScrollbarRounding);
    QUERY->doc_var(QUERY, "float     ScrollbarRounding");
    static t_CKINT ImGuiStyleVar_GrabMinSize = 20;
    QUERY->add_svar(QUERY, "int", "GrabMinSize", true,
                    &ImGuiStyleVar_GrabMinSize);
    QUERY->doc_var(QUERY, "float     GrabMinSize");
    static t_CKINT ImGuiStyleVar_GrabRounding = 21;
    QUERY->add_svar(QUERY, "int", "GrabRounding", true,
                    &ImGuiStyleVar_GrabRounding);
    QUERY->doc_var(QUERY, "float     GrabRounding");
    static t_CKINT ImGuiStyleVar_TabRounding = 22;
    QUERY->add_svar(QUERY, "int", "TabRounding", true,
                    &ImGuiStyleVar_TabRounding);
    QUERY->doc_var(QUERY, "float     TabRounding");
    static t_CKINT ImGuiStyleVar_TabBorderSize = 23;
    QUERY->add_svar(QUERY, "int", "TabBorderSize", true,
                    &ImGuiStyleVar_TabBorderSize);
    QUERY->doc_var(QUERY, "float     TabBorderSize");
    static t_CKINT ImGuiStyleVar_TabBarBorderSize = 24;
    QUERY->add_svar(QUERY, "int", "TabBarBorderSize", true,
                    &ImGuiStyleVar_TabBarBorderSize);
    QUERY->doc_var(QUERY, "float     TabBarBorderSize");
    static t_CKINT ImGuiStyleVar_TableAngledHeadersAngle = 25;
    QUERY->add_svar(QUERY, "int", "TableAngledHeadersAngle", true,
                    &ImGuiStyleVar_TableAngledHeadersAngle);
    QUERY->doc_var(QUERY, "float     TableAngledHeadersAngle");
    static t_CKINT ImGuiStyleVar_TableAngledHeadersTextAlign = 26;
    QUERY->add_svar(QUERY, "int", "TableAngledHeadersTextAlign", true,
                    &ImGuiStyleVar_TableAngledHeadersTextAlign);
    QUERY->doc_var(QUERY, "ImVec2  TableAngledHeadersTextAlign");
    static t_CKINT ImGuiStyleVar_ButtonTextAlign = 27;
    QUERY->add_svar(QUERY, "int", "ButtonTextAlign", true,
                    &ImGuiStyleVar_ButtonTextAlign);
    QUERY->doc_var(QUERY, "ImVec2    ButtonTextAlign");
    static t_CKINT ImGuiStyleVar_SelectableTextAlign = 28;
    QUERY->add_svar(QUERY, "int", "SelectableTextAlign", true,
                    &ImGuiStyleVar_SelectableTextAlign);
    QUERY->doc_var(QUERY, "ImVec2    SelectableTextAlign");
    static t_CKINT ImGuiStyleVar_SeparatorTextBorderSize = 29;
    QUERY->add_svar(QUERY, "int", "SeparatorTextBorderSize", true,
                    &ImGuiStyleVar_SeparatorTextBorderSize);
    QUERY->doc_var(QUERY, "float     SeparatorTextBorderSize");
    static t_CKINT ImGuiStyleVar_SeparatorTextAlign = 30;
    QUERY->add_svar(QUERY, "int", "SeparatorTextAlign", true,
                    &ImGuiStyleVar_SeparatorTextAlign);
    QUERY->doc_var(QUERY, "ImVec2    SeparatorTextAlign");
    static t_CKINT ImGuiStyleVar_SeparatorTextPadding = 31;
    QUERY->add_svar(QUERY, "int", "SeparatorTextPadding", true,
                    &ImGuiStyleVar_SeparatorTextPadding);
    QUERY->doc_var(QUERY, "ImVec2    SeparatorTextPadding");
    static t_CKINT ImGuiStyleVar_DockingSeparatorSize = 32;
    QUERY->add_svar(QUERY, "int", "DockingSeparatorSize", true,
                    &ImGuiStyleVar_DockingSeparatorSize);
    QUERY->doc_var(QUERY, "float     DockingSeparatorSize");
    static t_CKINT ImGuiStyleVar_COUNT = 33;
    QUERY->add_svar(QUERY, "int", "COUNT", true, &ImGuiStyleVar_COUNT);
    QUERY->end_class(QUERY);

    QUERY->begin_class(QUERY, "UI_ButtonFlags", "Object");
    QUERY->doc_class(
      QUERY, "Flags for InvisibleButton() [extended in imgui_internal.h].\n");
    static t_CKINT ImGuiButtonFlags_None = 0;
    QUERY->add_svar(QUERY, "int", "None", true, &ImGuiButtonFlags_None);
    static t_CKINT ImGuiButtonFlags_MouseButtonLeft = 1;
    QUERY->add_svar(QUERY, "int", "MouseButtonLeft", true,
                    &ImGuiButtonFlags_MouseButtonLeft);
    QUERY->doc_var(QUERY, "React on left mouse button (default)");
    static t_CKINT ImGuiButtonFlags_MouseButtonRight = 2;
    QUERY->add_svar(QUERY, "int", "MouseButtonRight", true,
                    &ImGuiButtonFlags_MouseButtonRight);
    QUERY->doc_var(QUERY, "React on right mouse button");
    static t_CKINT ImGuiButtonFlags_MouseButtonMiddle = 4;
    QUERY->add_svar(QUERY, "int", "MouseButtonMiddle", true,
                    &ImGuiButtonFlags_MouseButtonMiddle);
    QUERY->doc_var(QUERY, "React on center mouse button");
    QUERY->end_class(QUERY);

    QUERY->begin_class(QUERY, "UI_Direction", "Object");
    QUERY->doc_class(QUERY, "A cardinal direction.\n");
    static t_CKINT ImGuiDir_None = -1;
    QUERY->add_svar(QUERY, "int", "None", true, &ImGuiDir_None);
    static t_CKINT ImGuiDir_Left = 0;
    QUERY->add_svar(QUERY, "int", "Left", true, &ImGuiDir_Left);
    static t_CKINT ImGuiDir_Right = 1;
    QUERY->add_svar(QUERY, "int", "Right", true, &ImGuiDir_Right);
    static t_CKINT ImGuiDir_Up = 2;
    QUERY->add_svar(QUERY, "int", "Up", true, &ImGuiDir_Up);
    static t_CKINT ImGuiDir_Down = 3;
    QUERY->add_svar(QUERY, "int", "Down", true, &ImGuiDir_Down);
    static t_CKINT ImGuiDir_COUNT = 4;
    QUERY->add_svar(QUERY, "int", "COUNT", true, &ImGuiDir_COUNT);
    QUERY->end_class(QUERY);

    QUERY->begin_class(QUERY, "UI_ComboFlags", "Object");
    QUERY->doc_class(QUERY, "Flags for ImGui::BeginCombo().\n");
    static t_CKINT ImGuiComboFlags_None = 0;
    QUERY->add_svar(QUERY, "int", "None", true, &ImGuiComboFlags_None);
    static t_CKINT ImGuiComboFlags_PopupAlignLeft = 1;
    QUERY->add_svar(QUERY, "int", "PopupAlignLeft", true,
                    &ImGuiComboFlags_PopupAlignLeft);
    QUERY->doc_var(QUERY, "Align the popup toward the left by default");
    static t_CKINT ImGuiComboFlags_HeightSmall = 2;
    QUERY->add_svar(QUERY, "int", "HeightSmall", true,
                    &ImGuiComboFlags_HeightSmall);
    QUERY->doc_var(
      QUERY,
      "Max ~4 items visible. Tip: If you want your combo popup to be a "
      "specific size you can use SetNextWindowSizeConstraints() prior to "
      "calling BeginCombo()");
    static t_CKINT ImGuiComboFlags_HeightRegular = 4;
    QUERY->add_svar(QUERY, "int", "HeightRegular", true,
                    &ImGuiComboFlags_HeightRegular);
    QUERY->doc_var(QUERY, "Max ~8 items visible (default)");
    static t_CKINT ImGuiComboFlags_HeightLarge = 8;
    QUERY->add_svar(QUERY, "int", "HeightLarge", true,
                    &ImGuiComboFlags_HeightLarge);
    QUERY->doc_var(QUERY, "Max ~20 items visible");
    static t_CKINT ImGuiComboFlags_HeightLargest = 16;
    QUERY->add_svar(QUERY, "int", "HeightLargest", true,
                    &ImGuiComboFlags_HeightLargest);
    QUERY->doc_var(QUERY, "As many fitting items as possible");
    static t_CKINT ImGuiComboFlags_NoArrowButton = 32;
    QUERY->add_svar(QUERY, "int", "NoArrowButton", true,
                    &ImGuiComboFlags_NoArrowButton);
    QUERY->doc_var(
      QUERY, "Display on the preview box without the square arrow button");
    static t_CKINT ImGuiComboFlags_NoPreview = 64;
    QUERY->add_svar(QUERY, "int", "NoPreview", true,
                    &ImGuiComboFlags_NoPreview);
    QUERY->doc_var(QUERY, "Display only a square arrow button");
    static t_CKINT ImGuiComboFlags_WidthFitPreview = 128;
    QUERY->add_svar(QUERY, "int", "WidthFitPreview", true,
                    &ImGuiComboFlags_WidthFitPreview);
    QUERY->doc_var(QUERY, "Width dynamically calculated from preview contents");
    QUERY->end_class(QUERY);

    QUERY->begin_class(QUERY, "UI_SliderFlags", "Object");
    QUERY->doc_class(
      QUERY,
      "Flags for DragFloat(), DragInt(), SliderFloat(), SliderInt() etc..\nWe "
      "use the same sets of flags for DragXXX() and SliderXXX() functions as "
      "the features are the same and it makes it easier to swap them..\n(Those "
      "are per-item flags. There are shared flags in ImGuiIO: "
      "io.ConfigDragClickToInputText).\n");
    static t_CKINT ImGuiSliderFlags_None = 0;
    QUERY->add_svar(QUERY, "int", "None", true, &ImGuiSliderFlags_None);
    static t_CKINT ImGuiSliderFlags_AlwaysClamp = 16;
    QUERY->add_svar(QUERY, "int", "AlwaysClamp", true,
                    &ImGuiSliderFlags_AlwaysClamp);
    QUERY->doc_var(
      QUERY,
      "Clamp value to min/max bounds when input manually with CTRL+Click. By "
      "default CTRL+Click allows going out of bounds.");
    static t_CKINT ImGuiSliderFlags_Logarithmic = 32;
    QUERY->add_svar(QUERY, "int", "Logarithmic", true,
                    &ImGuiSliderFlags_Logarithmic);
    QUERY->doc_var(QUERY,
                   "Make the widget logarithmic (linear otherwise). Consider "
                   "using ImGuiSliderFlags_NoRoundToFormat with this if using "
                   "a format-string with small amount of digits.");
    static t_CKINT ImGuiSliderFlags_NoRoundToFormat = 64;
    QUERY->add_svar(QUERY, "int", "NoRoundToFormat", true,
                    &ImGuiSliderFlags_NoRoundToFormat);
    QUERY->doc_var(
      QUERY,
      "Disable rounding underlying value to match precision of the display "
      "format string (e.g. %.3f values are rounded to those 3 digits)");
    static t_CKINT ImGuiSliderFlags_NoInput = 128;
    QUERY->add_svar(QUERY, "int", "NoInput", true, &ImGuiSliderFlags_NoInput);
    QUERY->doc_var(QUERY,
                   "Disable CTRL+Click or Enter key allowing to input text "
                   "directly into the widget");
    QUERY->end_class(QUERY);

    // Callbacks ----------------------------------------------------------

    BEGIN_CLASS("UI_Callback", "Object");
    DOC_CLASS(
      "Base class for ImGui callback functions. Don't use this class "
      "directly.");
    CTOR(ui_callback_ctor);
    DTOR(ui_callback_dtor);
    END_CLASS();

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

    QUERY->begin_class(QUERY, "UI_SizeCallback", "UI_Callback");
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

    BEGIN_CLASS("UI_ComboCallback", "UI_Callback");
    MFUN(ui_combo_callback, "void", "handler");
    ARG("int", "idx");
    END_CLASS();

    chugin_setVTableOffset(&ui_combo_callback_vt_offset, "UI_ComboCallback",
                           "handler");

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

    // Content region -------------------------------------------------------
    QUERY->add_sfun(QUERY, ui_GetContentRegionAvail, "vec2",
                    "getContentRegionAvail");
    QUERY->doc_func(QUERY,
                    "equivalent to GetContentRegionMax() - GetCursorPos()");

    QUERY->add_sfun(QUERY, ui_GetContentRegionMax, "vec2",
                    "getContentRegionMax");
    QUERY->doc_func(
      QUERY,
      "current content boundaries (typically window boundaries including "
      "scrolling, or current column boundaries), in windows coordinates");

    QUERY->add_sfun(QUERY, ui_GetWindowContentRegionMin, "vec2",
                    "getWindowContentRegionMin");
    QUERY->doc_func(QUERY,
                    "content boundaries min for the full window (roughly "
                    "(0,0)-Scroll), in window coordinates");

    QUERY->add_sfun(QUERY, ui_GetWindowContentRegionMax, "vec2",
                    "getWindowContentRegionMax");
    QUERY->doc_func(QUERY,
                    "content boundaries max for the full window (roughly "
                    "(0,0)+Size-Scroll) where Size can be overridden with "
                    "SetNextWindowContentSize(), in window coordinates");

    // Windows Scrolling ----------------------------------------------------

    QUERY->add_sfun(QUERY, ui_GetScrollX, "float", "getScrollX");
    QUERY->doc_func(QUERY, "get scrolling amount [0 .. GetScrollMaxX()]");

    QUERY->add_sfun(QUERY, ui_GetScrollY, "float", "getScrollY");
    QUERY->doc_func(QUERY, "get scrolling amount [0 .. GetScrollMaxY()]");

    QUERY->add_sfun(QUERY, ui_SetScrollX, "void", "setScrollX");
    QUERY->add_arg(QUERY, "float", "scroll_x");
    QUERY->doc_func(QUERY, "set scrolling amount [0 .. GetScrollMaxX()]");

    QUERY->add_sfun(QUERY, ui_SetScrollY, "void", "setScrollY");
    QUERY->add_arg(QUERY, "float", "scroll_y");
    QUERY->doc_func(QUERY, "set scrolling amount [0 .. GetScrollMaxY()]");

    QUERY->add_sfun(QUERY, ui_GetScrollMaxX, "float", "getScrollMaxX");
    QUERY->doc_func(QUERY,
                    "get maximum scrolling amount ~~ ContentSize.x - "
                    "WindowSize.x - DecorationsSize.x");

    QUERY->add_sfun(QUERY, ui_GetScrollMaxY, "float", "getScrollMaxY");
    QUERY->doc_func(QUERY,
                    "get maximum scrolling amount ~~ ContentSize.y - "
                    "WindowSize.y - DecorationsSize.y");

    QUERY->add_sfun(QUERY, ui_SetScrollHereX, "void", "setScrollHereX");
    QUERY->add_arg(QUERY, "float", "center_x_ratio");
    QUERY->doc_func(QUERY,
                    "adjust scrolling amount to make current cursor position "
                    "visible. center_x_ratio=0.0: left, 0.5: center, 1.0: "
                    "right. When using to make a \"default/current item\" "
                    "visible, consider using SetItemDefaultFocus() instead.");

    QUERY->add_sfun(QUERY, ui_SetScrollHereY, "void", "setScrollHereY");
    QUERY->add_arg(QUERY, "float", "center_y_ratio");
    QUERY->doc_func(QUERY,
                    "adjust scrolling amount to make current cursor position "
                    "visible. center_y_ratio=0.0: top, 0.5: center, 1.0: "
                    "bottom. When using to make a \"default/current item\" "
                    "visible, consider using SetItemDefaultFocus() instead.");

    QUERY->add_sfun(QUERY, ui_SetScrollFromPosX, "void", "setScrollFromPosX");
    QUERY->add_arg(QUERY, "float", "local_x");
    QUERY->add_arg(QUERY, "float", "center_x_ratio");
    QUERY->doc_func(
      QUERY,
      "adjust scrolling amount to make given position visible. Generally "
      "GetCursorStartPos() + offset to compute a valid position.");

    QUERY->add_sfun(QUERY, ui_SetScrollFromPosY, "void", "setScrollFromPosY");
    QUERY->add_arg(QUERY, "float", "local_y");
    QUERY->add_arg(QUERY, "float", "center_y_ratio");
    QUERY->doc_func(
      QUERY,
      "adjust scrolling amount to make given position visible. Generally "
      "GetCursorStartPos() + offset to compute a valid position.");

    // Parameters stacks (shared) ------------------------------------------

    QUERY->add_sfun(QUERY, ui_PushStyleColorImVec4, "void", "pushStyleColor");
    QUERY->add_arg(QUERY, "int", "idx" /*ImGuiCol*/);
    QUERY->add_arg(QUERY, "vec4", "color");
    QUERY->doc_func(QUERY, "parameter idx an enum of type UI_Color");

    QUERY->add_sfun(QUERY, ui_PopStyleColor, "void", "popStyleColor");
    QUERY->doc_func(QUERY, "implied count = 1");

    QUERY->add_sfun(QUERY, ui_PopStyleColorEx, "void", "popStyleColor");
    QUERY->add_arg(QUERY, "int", "count");

    QUERY->add_sfun(QUERY, ui_PushStyleVar, "void", "pushStyleVar");
    QUERY->add_arg(QUERY, "int", "idx" /*ImGuiStyleVar*/);
    QUERY->add_arg(QUERY, "float", "val");
    QUERY->doc_func(QUERY,
                    "modify a style float variable. always use this if "
                    "you modify the style after NewFrame(). \n Parameter `idx` "
                    "is an enum of type UI_StyleVar");

    QUERY->add_sfun(QUERY, ui_PushStyleVarImVec2, "void", "pushStyleVar");
    QUERY->add_arg(QUERY, "int", "idx" /*ImGuiStyleVar*/);
    QUERY->add_arg(QUERY, "vec2", "val");
    QUERY->doc_func(QUERY,
                    "modify a style ImVec2 variable. always use this if "
                    "you modify the style after NewFrame(). \n Parameter `idx` "
                    "is an enum of type UI_StyleVar");

    QUERY->add_sfun(QUERY, ui_PopStyleVar, "void", "popStyleVar");
    QUERY->doc_func(QUERY, "implied count = 1");

    QUERY->add_sfun(QUERY, ui_PopStyleVarEx, "void", "popStyleVar");
    QUERY->add_arg(QUERY, "int", "count");

    QUERY->add_sfun(QUERY, ui_PushTabStop, "void", "pushTabStop");
    QUERY->add_arg(QUERY, "int", "tab_stop");
    QUERY->doc_func(QUERY,
                    "allow focusing using TAB/Shift-TAB, enabled by default "
                    "but you can disable it for certain widgets");

    QUERY->add_sfun(QUERY, ui_PopTabStop, "void", "popTabStop");

    QUERY->add_sfun(QUERY, ui_PushButtonRepeat, "void", "pushButtonRepeat");
    QUERY->add_arg(QUERY, "int", "repeat");
    QUERY->doc_func(QUERY,
                    "in 'repeat' mode, Button*() functions return repeated "
                    "true in a typematic manner (using "
                    "io.KeyRepeatDelay/io.KeyRepeatRate setting). Note that "
                    "you can call IsItemActive() after any Button() to tell if "
                    "the button is held in the current frame.");

    QUERY->add_sfun(QUERY, ui_PopButtonRepeat, "void", "popButtonRepeat");

    // Style read access ----------------------------------------------------
    QUERY->add_sfun(QUERY, ui_GetFontSize, "float", "getFontSize");
    QUERY->doc_func(QUERY,
                    "get current font size (= height in pixels) of "
                    "current font with current scale applied");

    QUERY->add_sfun(QUERY, ui_GetFontTexUvWhitePixel, "vec2",
                    "getFontTexUvWhitePixel");
    QUERY->doc_func(QUERY,
                    "get UV coordinate for a white pixel, useful to "
                    "draw custom shapes via the ImDrawList API");

    QUERY->add_sfun(QUERY, ui_GetColorU32, "int", "getColorU32");
    QUERY->add_arg(QUERY, "int", "idx" /*ImGuiCol*/);
    QUERY->doc_func(QUERY,
                    "implied alpha_mul = 1.0f. retrieve given style color with "
                    "style alpha applied and optional extra alpha multiplier, "
                    "packed as a 32-bit value suitable for ImDrawList\n"
                    "Parameter `idx` is an enum of type UI_Color");

    QUERY->add_sfun(QUERY, ui_GetColorU32Ex, "int", "getColorU32");
    QUERY->add_arg(QUERY, "int", "idx" /*ImGuiCol*/);
    QUERY->add_arg(QUERY, "float", "alpha_mul");
    QUERY->doc_func(QUERY,
                    "retrieve given style color with style alpha applied and "
                    "optional extra alpha multiplier, packed as a 32-bit value "
                    "suitable for ImDrawList\n"
                    "Parameter `idx` is an enum of type UI_color");

    QUERY->add_sfun(QUERY, ui_GetColorU32ImVec4, "int", "getColorU32");
    QUERY->add_arg(QUERY, "vec4", "col");
    QUERY->doc_func(
      QUERY,
      "retrieve given color with style alpha applied, packed as a "
      "32-bit value suitable for ImDrawList");

    QUERY->add_sfun(QUERY, ui_GetStyleColorVec4, "vec4", "getStyleColorVec4");
    QUERY->add_arg(QUERY, "int", "idx" /*ImGuiCol*/);
    QUERY->doc_func(QUERY,
                    "retrieve style color as stored in ImGuiStyle structure. "
                    "use to feed back into PushStyleColor(), otherwise use "
                    "GetColorU32() to get style color with style alpha baked "
                    "in.\n"
                    "Parameter `idx` is an enum of type UI_Color");

    // Layout cursor positioning
    QUERY->add_sfun(QUERY, ui_GetCursorScreenPos, "vec2", "getCursorScreenPos");
    QUERY->doc_func(QUERY,
                    "cursor position in absolute coordinates (prefer using "
                    "this, also more useful to work with ImDrawList API)");

    QUERY->add_sfun(QUERY, ui_SetCursorScreenPos, "vec2", "setCursorScreenPos");
    QUERY->add_arg(QUERY, "vec2", "pos");
    QUERY->doc_func(QUERY, "cursor position in absolute coordinates");

    QUERY->add_sfun(QUERY, ui_GetCursorPos, "vec2", "getCursorPos");
    QUERY->doc_func(QUERY,
                    "cursor position in window coordinates (relative to window "
                    "position)");

    QUERY->add_sfun(QUERY, ui_GetCursorPosX, "vec2", "getCursorPosX");

    QUERY->add_sfun(QUERY, ui_GetCursorPosY, "vec2", "getCursorPosY");

    QUERY->add_sfun(QUERY, ui_SetCursorPos, "void", "setCursorPos");
    QUERY->add_arg(QUERY, "vec2", "local_pos");

    QUERY->add_sfun(QUERY, ui_SetCursorPosX, "void", "setCursorPosX");
    QUERY->add_arg(QUERY, "float", "local_x");

    QUERY->add_sfun(QUERY, ui_SetCursorPosY, "void", "setCursorPosY");
    QUERY->add_arg(QUERY, "float", "local_y");

    QUERY->add_sfun(QUERY, ui_GetCursorStartPos, "vec2", "getCursorStartPos");
    QUERY->doc_func(QUERY, "initial cursor position, in window coordinates");

    // Other layout functions ------------------------------------------------

    QUERY->add_sfun(QUERY, ui_Separator, "void", "separator");
    QUERY->doc_func(QUERY,
                    "separator, generally horizontal. inside a menu "
                    "bar or in horizontal layout mode, this becomes a "
                    "vertical separator.");

    QUERY->add_sfun(QUERY, ui_SameLine, "void", "sameLine");
    QUERY->doc_func(QUERY,
                    "implied offset_from_start_x = 0.0f, spacing = -1.0f");

    QUERY->add_sfun(QUERY, ui_SameLineEx, "void", "sameLine");
    QUERY->add_arg(QUERY, "float", "offset_from_start_x");
    QUERY->add_arg(QUERY, "float", "spacing");
    QUERY->doc_func(QUERY,
                    "call between widgets or groups to layout them "
                    "horizontally. X position given in window coordinates.");

    QUERY->add_sfun(QUERY, ui_NewLine, "void", "newLine");
    QUERY->doc_func(QUERY,
                    "undo a SameLine() or force a new line when in a "
                    "horizontal-layout context.");

    QUERY->add_sfun(QUERY, ui_Spacing, "void", "spacing");
    QUERY->doc_func(QUERY, "add vertical spacing.");

    QUERY->add_sfun(QUERY, ui_Dummy, "void", "dummy");
    QUERY->add_arg(QUERY, "vec2", "size");
    QUERY->doc_func(QUERY,
                    "add a dummy item of given size. unlike InvisibleButton(), "
                    "Dummy() won't take the mouse click or be navigable into.");

    QUERY->add_sfun(QUERY, ui_Indent, "void", "indent");
    QUERY->doc_func(QUERY, "implied indent_w = 0.0f");

    QUERY->add_sfun(QUERY, ui_IndentEx, "void", "indent");
    QUERY->add_arg(QUERY, "float", "indent_w");
    QUERY->doc_func(QUERY,
                    "move content position toward the right, by indent_w, or "
                    "style.IndentSpacing if indent_w <= 0");

    QUERY->add_sfun(QUERY, ui_Unindent, "void", "unindent");
    QUERY->doc_func(QUERY, "implied indent_w = 0.0f");

    QUERY->add_sfun(QUERY, ui_UnindentEx, "void", "unindent");
    QUERY->add_arg(QUERY, "float", "indent_w");
    QUERY->doc_func(QUERY,
                    "move content position back to the left, by indent_w, or "
                    "style.IndentSpacing if indent_w <= 0");

    QUERY->add_sfun(QUERY, ui_BeginGroup, "void", "beginGroup");
    QUERY->doc_func(QUERY, "lock horizontal starting position");

    QUERY->add_sfun(QUERY, ui_EndGroup, "void", "endGroup");
    QUERY->doc_func(
      QUERY,
      "unlock horizontal starting position + capture the whole "
      "group bounding box into one \"item\" (so you can use "
      "IsItemHovered() or layout primitives such as SameLine() on "
      "whole group, etc.)");

    QUERY->add_sfun(QUERY, ui_AlignTextToFramePadding, "void",
                    "alignTextToFramePadding");
    QUERY->doc_func(QUERY,
                    "vertically align upcoming text baseline to FramePadding.y "
                    "so that it will align properly to regularly framed items "
                    "(call if you have text on a line before a framed item)");

    QUERY->add_sfun(QUERY, ui_GetTextLineHeight, "float", "getTextLineHeight");
    QUERY->doc_func(QUERY, "~ FontSize");

    QUERY->add_sfun(QUERY, ui_GetTextLineHeightWithSpacing, "float",
                    "getTextLineHeightWithSpacing");
    QUERY->doc_func(QUERY,
                    "~ FontSize + style.ItemSpacing.y (distance in pixels "
                    "between 2 consecutive lines of text)");

    QUERY->add_sfun(QUERY, ui_GetFrameHeight, "float", "getFrameHeight");
    QUERY->doc_func(QUERY, "~ FontSize + style.FramePadding.y * 2");

    QUERY->add_sfun(QUERY, ui_GetFrameHeightWithSpacing, "float",
                    "getFrameHeightWithSpacing");
    QUERY->doc_func(QUERY,
                    "~ FontSize + style.FramePadding.y * 2 + "
                    "style.ItemSpacing.y (distance in pixels between 2 "
                    "consecutive lines of framed widgets)");

    // ID stack/scopes -------------------------------------------------------

    QUERY->add_sfun(QUERY, ui_PushID, "void", "pushID");
    QUERY->add_arg(QUERY, "string", "str_id");
    QUERY->doc_func(QUERY,
                    "push string into the ID stack (will hash string)."
                    "Read the FAQ (docs/FAQ.md or http://dearimgui.com/faq) "
                    "for more details about how ID are handled in dear imgui.");

    QUERY->add_sfun(QUERY, ui_PushIDStr, "void", "pushID");
    QUERY->add_arg(QUERY, "string", "str_id_begin");
    QUERY->add_arg(QUERY, "string", "str_id_end");
    QUERY->doc_func(QUERY, "push string into the ID stack (will hash string).");

    QUERY->add_sfun(QUERY, ui_PushIDInt, "void", "pushID");
    QUERY->add_arg(QUERY, "int", "int_id");
    QUERY->doc_func(QUERY,
                    "push integer into the ID stack (will hash integer).");

    QUERY->add_sfun(QUERY, ui_PopID, "void", "popID");
    QUERY->doc_func(QUERY, "pop from the ID stack.");

    QUERY->add_sfun(QUERY, ui_GetID, "int", "getID");
    QUERY->add_arg(QUERY, "string", "str_id");
    QUERY->doc_func(QUERY,
                    "calculate unique ID (hash of whole ID stack + given "
                    "parameter). e.g. if you want to query into ImGuiStorage "
                    "yourself");

    QUERY->add_sfun(QUERY, ui_GetIDStr, "int", "getID");
    QUERY->add_arg(QUERY, "string", "str_id_begin");
    QUERY->add_arg(QUERY, "string", "str_id_end");

    // Widgets: Text ---------------------------------------------------------

    QUERY->add_sfun(QUERY, ui_TextUnformatted, "void", "text");
    QUERY->add_arg(QUERY, "string", "text");
    QUERY->doc_func(QUERY, "implied text_end = NULL");

    QUERY->add_sfun(QUERY, ui_TextUnformattedEx, "void", "text");
    QUERY->add_arg(QUERY, "string", "text");
    QUERY->add_arg(QUERY, "string", "text_end");
    QUERY->doc_func(QUERY,
                    "raw text without formatting. Roughly equivalent to "
                    "Text(\"%s\", text) but: A) doesn't require null "
                    "terminated string if 'text_end' is specified, B) it's "
                    "faster, no memory copy is done, no buffer size limits, "
                    "recommended for long chunks of text.");

    QUERY->add_sfun(QUERY, ui_TextColoredUnformatted, "void", "textColored");
    QUERY->add_arg(QUERY, "vec4", "col");
    QUERY->add_arg(QUERY, "string", "text");
    QUERY->doc_func(QUERY,
                    "shortcut for PushStyleColor(ImGuiCol_Text, col); "
                    "Text(fmt, ...); PopStyleColor();");

    QUERY->add_sfun(QUERY, ui_TextDisabledUnformatted, "void", "textDisabled");
    QUERY->add_arg(QUERY, "string", "text");
    QUERY->doc_func(QUERY,
                    "shortcut for PushStyleColor(ImGuiCol_Text, "
                    "style.Colors[ImGuiCol_TextDisabled]); Text(fmt, ...); "
                    "PopStyleColor();");

    QUERY->add_sfun(QUERY, ui_TextWrappedUnformatted, "void", "textWrapped");
    QUERY->add_arg(QUERY, "string", "text");
    QUERY->doc_func(
      QUERY,
      "shortcut for PushTextWrapPos(0.0f); Text(fmt, ...); "
      "PopTextWrapPos();. Note that this won't work on an "
      "auto-resizing window if there's no other widgets to extend "
      "the window width, you may need to set a size using "
      "SetNextWindowSize().");

    QUERY->add_sfun(QUERY, ui_LabelTextUnformatted, "void", "labelText");
    QUERY->add_arg(QUERY, "string", "label");
    QUERY->add_arg(QUERY, "string", "text");
    QUERY->doc_func(QUERY,
                    "display text+label aligned the same way as value+label "
                    "widgets");

    QUERY->add_sfun(QUERY, ui_BulletTextUnformatted, "void", "bulletText");
    QUERY->add_arg(QUERY, "string", "text");
    QUERY->doc_func(QUERY, "shortcut for Bullet()+Text()");

    QUERY->add_sfun(QUERY, ui_SeparatorText, "void", "separatorText");
    QUERY->add_arg(QUERY, "string", "label");
    QUERY->doc_func(QUERY, "currently: formatted text with an horizontal line");

    // Widgets: Main ---------------------------------------------------------

    QUERY->add_sfun(QUERY, ui_Button, "int", "button");
    QUERY->add_arg(QUERY, "string", "label");
    QUERY->doc_func(QUERY, "implied size = ImVec2(0, 0)");

    QUERY->add_sfun(QUERY, ui_ButtonEx, "int", "button");
    QUERY->add_arg(QUERY, "string", "label");
    QUERY->add_arg(QUERY, "vec2", "size");

    QUERY->add_sfun(QUERY, ui_SmallButton, "int", "smallButton");
    QUERY->add_arg(QUERY, "string", "label");
    QUERY->doc_func(QUERY, "button with (FramePadding.y == 0)");

    QUERY->add_sfun(QUERY, ui_InvisibleButton, "int", "invisibleButton");
    QUERY->add_arg(QUERY, "string", "str_id");
    QUERY->add_arg(QUERY, "vec2", "size");
    QUERY->add_arg(QUERY, "int", "flags");
    QUERY->doc_func(QUERY,
                    "flexible button behavior without the visuals, frequently "
                    "useful to build custom behaviors using the public api "
                    "(along with IsItemActive, IsItemHovered, etc.)"
                    "param `flags` is an enum of type UI_ButtonFlags");

    QUERY->add_sfun(QUERY, ui_ArrowButton, "int", "arrowButton");
    QUERY->add_arg(QUERY, "string", "str_id");
    QUERY->add_arg(QUERY, "int", "direction");
    QUERY->doc_func(QUERY,
                    "square button with an arrow shape. param `direction` is "
                    "an enum of type UI_Direction");

    QUERY->add_sfun(QUERY, ui_Checkbox, "int", "checkbox");
    QUERY->add_arg(QUERY, "string", "label");
    QUERY->add_arg(QUERY, "UI_Bool", "v");

    QUERY->add_sfun(QUERY, ui_CheckboxFlagsIntPtr, "int", "checkboxFlags");
    QUERY->add_arg(QUERY, "string", "label");
    QUERY->add_arg(QUERY, "UI_Int", "flags");
    QUERY->add_arg(QUERY, "int", "flags_value");

    QUERY->add_sfun(QUERY, ui_RadioButton, "int", "radioButton");
    QUERY->add_arg(QUERY, "string", "label");
    QUERY->add_arg(QUERY, "int", "active");
    QUERY->doc_func(QUERY,
                    "use with e.g. if (RadioButton(\"one\", my_value==1)) { "
                    "my_value = 1; }");

    QUERY->add_sfun(QUERY, ui_RadioButtonIntPtr, "int", "radioButton");
    QUERY->add_arg(QUERY, "string", "label");
    QUERY->add_arg(QUERY, "UI_Int", "v");
    QUERY->add_arg(QUERY, "int", "v_button");
    QUERY->doc_func(QUERY,
                    "shortcut to handle the above pattern when value is an "
                    "integer");

    QUERY->add_sfun(QUERY, ui_ProgressBar, "void", "progressBar");
    QUERY->add_arg(QUERY, "float", "fraction");
    QUERY->add_arg(QUERY, "vec2", "size_arg");
    QUERY->add_arg(QUERY, "string", "overlay");

    QUERY->add_sfun(QUERY, ui_Bullet, "void", "bullet");
    QUERY->doc_func(QUERY,
                    "draw a small circle + keep the cursor on the same line. "
                    "advance cursor x position by GetTreeNodeToLabelSpacing(), "
                    "same distance that TreeNode() uses");

    // Widgets: Combo --------------------------------------------------------

    QUERY->add_sfun(QUERY, ui_BeginCombo, "int", "beginCombo");
    QUERY->add_arg(QUERY, "string", "label");
    QUERY->add_arg(QUERY, "string", "preview_value");
    QUERY->add_arg(QUERY, "int", "flags");
    QUERY->doc_func(
      QUERY,
      "The BeginCombo()/EndCombo() api allows you to manage your contents and "
      "selection state however you want it, by creating e.g. Selectable() "
      "items.\n"
      "// - The old Combo() api are helpers over BeginCombo()/EndCombo() which "
      "are kept available for convenience purpose. This is analogous to how "
      "ListBox are created.\n"
      "`flags` param is an enum of type UI_ComboFlags");

    QUERY->add_sfun(QUERY, ui_EndCombo, "void", "endCombo");
    QUERY->doc_func(QUERY,
                    "only call EndCombo() if BeginCombo() returns true!");

    QUERY->add_sfun(QUERY, ui_ComboChar, "int", "combo");
    QUERY->add_arg(QUERY, "string", "label");
    QUERY->add_arg(QUERY, "UI_Int", "current_item");
    QUERY->add_arg(QUERY, "string[]", "items");
    QUERY->doc_func(QUERY, "implied popup_max_height_in_items = -1");

    QUERY->add_sfun(QUERY, ui_ComboCharEx, "int", "combo");
    QUERY->add_arg(QUERY, "string", "label");
    QUERY->add_arg(QUERY, "UI_Int", "current_item");
    QUERY->add_arg(QUERY, "string[]", "items");
    QUERY->add_arg(QUERY, "int", "popup_max_height_in_items");

    // TODO: ui_Combo doesn't work with chuck strings
    // cannot seem to escape \0 (gets copied literally) by
    // API->object->str(Chuck_String*)

    // QUERY->add_sfun(QUERY, ui_Combo, "int", "combo");
    // QUERY->add_arg(QUERY, "string", "label");
    // QUERY->add_arg(QUERY, "UI_Int", "current_item");
    // QUERY->add_arg(QUERY, "string", "items_separated_by_zeros");
    // QUERY->doc_func(QUERY, "implied popup_max_height_in_items = -1");

    // SFUN(ui_ComboEx, "int", "combo");
    // ARG("string", "label");
    // ARG("UI_Int", "current_item");
    // ARG("string", "items_separated_by_zeros");
    // ARG("int", "popup_max_height_in_items");
    // DOC_FUNC(
    //   "Separate items with \\0 within a string, end "
    //   "item-list with \\0\\0. e.g. \"One\\0Two\\0Three\\0\\0\"");

    // Widgets: Drag ---------------------------------------------------------

    SFUN(ui_DragFloat, "int", "drag");
    ARG("string", "label");
    ARG("UI_Float", "v");
    DOC_FUNC(
      "Implied v_speed = 1.0f, v_min = 0.0f, v_max = 0.0f, format = "
      "\"%.3f\", flags = 0");

    SFUN(ui_DragFloatEx, "int", "drag");
    ARG("string", "label");
    ARG("UI_Float", "v");
    ARG("float", "v_speed");
    ARG("float", "v_min");
    ARG("float", "v_max");
    ARG("string", "format");
    ARG("int", "flags");
    DOC_FUNC(
      "If v_min >= v_max we have no bound. Parameter `flags` is an enum of "
      "type UI_SliderFlags");

    SFUN(ui_DragFloat2, "int", "drag");
    ARG("string", "label");
    ARG("UI_Float2", "v");
    DOC_FUNC(
      "Implied v_speed = 1.0f, v_min = 0.0f, v_max = 0.0f, format = "
      "\"%.3f\", flags = 0");

    SFUN(ui_DragFloat2Ex, "int", "drag");
    ARG("string", "label");
    ARG("UI_Float2", "v");
    ARG("float", "v_speed");
    ARG("float", "v_min");
    ARG("float", "v_max");
    ARG("string", "format");
    ARG("int", "flags");
    DOC_FUNC(
      "If v_min >= v_max we have no bound. Parameter `flags` is an enum of "
      "type UI_SliderFlags");

    SFUN(ui_DragFloat3, "int", "drag");
    ARG("string", "label");
    ARG("UI_Float3", "v");
    DOC_FUNC(
      "Implied v_speed = 1.0f, v_min = 0.0f, v_max = 0.0f, format = "
      "\"%.3f\", flags = 0");

    SFUN(ui_DragFloat3Ex, "int", "drag");
    ARG("string", "label");
    ARG("UI_Float3", "v");
    ARG("float", "v_speed");
    ARG("float", "v_min");
    ARG("float", "v_max");
    ARG("string", "format");
    ARG("int", "flags");
    DOC_FUNC(
      "If v_min >= v_max we have no bound. Parameter `flags` is an enum of "
      "type UI_SliderFlags");

    SFUN(ui_DragFloat4, "int", "drag");
    ARG("string", "label");
    ARG("UI_Float4", "v");
    DOC_FUNC(
      "Implied v_speed = 1.0f, v_min = 0.0f, v_max = 0.0f, format = "
      "\"%.3f\", flags = 0");

    SFUN(ui_DragFloat4Ex, "int", "drag");
    ARG("string", "label");
    ARG("UI_Float4", "v");
    ARG("float", "v_speed");
    ARG("float", "v_min");
    ARG("float", "v_max");
    ARG("string", "format");
    ARG("int", "flags");
    DOC_FUNC(
      "If v_min >= v_max we have no bound. Parameter `flags` is an enum of "
      "type UI_SliderFlags");

    SFUN(ui_DragFloatRange2, "int", "drag");
    ARG("string", "label");
    ARG("UI_Float", "v_current_min");
    ARG("UI_Float", "v_current_max");
    DOC_FUNC(
      "Implied v_speed = 1.0f, v_min = 0.0f, v_max = 0.0f, format = "
      "\"%.3f\", format_max = NULL, flags = 0");

    SFUN(ui_DragFloatRange2Ex, "int", "drag");
    ARG("string", "label");
    ARG("UI_Float", "v_current_min");
    ARG("UI_Float", "v_current_max");
    ARG("float", "v_speed");
    ARG("float", "v_min");
    ARG("float", "v_max");
    ARG("string", "format");
    ARG("string", "format_max");
    ARG("int", "flags");
    DOC_FUNC("Parameter `flags` is an enum of type UI_SliderFlags");

    SFUN(ui_DragInt, "int", "drag");
    ARG("string", "label");
    ARG("UI_Int", "v");
    DOC_FUNC(
      "Implied v_speed = 1.0f, v_min = 0, v_max = 0, format = \"%d\", flags = "
      "0");

    SFUN(ui_DragIntEx, "int", "drag");
    ARG("string", "label");
    ARG("UI_Int", "v");
    ARG("float", "v_speed");
    ARG("int", "v_min");
    ARG("int", "v_max");
    ARG("string", "format");
    ARG("int", "flags");
    DOC_FUNC(
      "If v_min >= v_max we have no bound. Parameter `flags` is an enum of "
      "type UI_SliderFlags");

    SFUN(ui_DragInt2, "int", "drag");
    ARG("string", "label");
    ARG("UI_Int2", "v");
    DOC_FUNC(
      "Implied v_speed = 1.0f, v_min = 0, v_max = 0, format = \"%d\", flags = "
      "0");

    SFUN(ui_DragInt2Ex, "int", "drag");
    ARG("string", "label");
    ARG("UI_Int2", "v");
    ARG("float", "v_speed");
    ARG("int", "v_min");
    ARG("int", "v_max");
    ARG("string", "format");
    ARG("int", "flags");
    DOC_FUNC(
      "If v_min >= v_max we have no bound. Parameter `flags` is an enum of "
      "type UI_SliderFlags");

    SFUN(ui_DragInt3, "int", "drag");
    ARG("string", "label");
    ARG("UI_Int3", "v");
    DOC_FUNC(
      "Implied v_speed = 1.0f, v_min = 0, v_max = 0, format = \"%d\", flags = "
      "0");

    SFUN(ui_DragInt3Ex, "int", "drag");
    ARG("string", "label");
    ARG("UI_Int3", "v");
    ARG("float", "v_speed");
    ARG("int", "v_min");
    ARG("int", "v_max");
    ARG("string", "format");
    ARG("int", "flags");
    DOC_FUNC(
      "If v_min >= v_max we have no bound. Parameter `flags` is an enum of "
      "type UI_SliderFlags");

    SFUN(ui_DragInt4, "int", "drag");
    ARG("string", "label");
    ARG("UI_Int4", "v");
    DOC_FUNC(
      "Implied v_speed = 1.0f, v_min = 0, v_max = 0, format = \"%d\", flags = "
      "0");

    SFUN(ui_DragInt4Ex, "int", "drag");
    ARG("string", "label");
    ARG("UI_Int4", "v");
    ARG("float", "v_speed");
    ARG("int", "v_min");
    ARG("int", "v_max");
    ARG("string", "format");
    ARG("int", "flags");
    DOC_FUNC(
      "If v_min >= v_max we have no bound. Parameter `flags` is an enum of "
      "type UI_SliderFlags");

    SFUN(ui_DragIntRange2, "int", "drag");
    ARG("string", "label");
    ARG("UI_Int", "v_current_min");
    ARG("UI_Int", "v_current_max");
    DOC_FUNC(
      "Implied v_speed = 1.0f, v_min = 0, v_max = 0, format = \"%d\", "
      "format_max = NULL, flags = 0");

    SFUN(ui_DragIntRange2Ex, "int", "drag");
    ARG("string", "label");
    ARG("UI_Int", "v_current_min");
    ARG("UI_Int", "v_current_max");
    ARG("float", "v_speed");
    ARG("int", "v_min");
    ARG("int", "v_max");
    ARG("string", "format");
    ARG("string", "format_max");
    ARG("int", "flags");
    DOC_FUNC("Parameter `flags` is an enum of type UI_SliderFlags");

    SFUN(ui_DragScalarN_CKINT, "int", "drag");
    ARG("string", "label");
    ARG("int[]", "data");

    SFUN(ui_DragScalarNEx_CKINT, "int", "drag");
    ARG("string", "label");
    ARG("int[]", "data");
    ARG("float", "v_speed");
    ARG("int", "v_min");
    ARG("int", "v_max");
    ARG("string", "format");
    ARG("int", "flags");
    DOC_FUNC("Parameter `flags` is an enum of type UI_SliderFlags");

    SFUN(ui_DragScalarN_CKFLOAT, "int", "drag");
    ARG("string", "label");
    ARG("float[]", "data");

    SFUN(ui_DragScalarNEx_CKFLOAT, "int", "drag");
    ARG("string", "label");
    ARG("float[]", "data");
    ARG("float", "v_speed");
    ARG("float", "v_min");
    ARG("float", "v_max");
    ARG("string", "format");
    ARG("int", "flags");
    DOC_FUNC("Parameter `flags` is an enum of type UI_SliderFlags");

    // Widgets: Slider -------------------------------------------------------

    SFUN(ui_SliderFloat, "int", "slider");
    ARG("string", "label");
    ARG("UI_Float", "v");
    ARG("float", "v_min");
    ARG("float", "v_max");
    DOC_FUNC("Implied format = \"%.3f\", flags = 0");

    SFUN(ui_SliderFloatEx, "int", "slider");
    ARG("string", "label");
    ARG("UI_Float", "v");
    ARG("float", "v_min");
    ARG("float", "v_max");
    ARG("string", "format");
    ARG("int", "flags");
    DOC_FUNC(
      "adjust format to decorate the value with a prefix or a suffix for in-"
      "slider labels or unit display. `flags` is an enum of type "
      "UI_SliderFlags");

    SFUN(ui_SliderAngle, "int", "sliderAngle");
    ARG("string", "label");
    ARG("UI_Float", "v_rad");
    DOC_FUNC(
      "Implied v_degrees_min = -360.0f, v_degrees_max = +360.0f, format = "
      "\"%.0f deg\", flags = 0");

    SFUN(ui_SliderAngleEx, "int", "sliderAngle");
    ARG("string", "label");
    ARG("UI_Float", "v_rad");
    ARG("float", "v_degrees_min");
    ARG("float", "v_degrees_max");
    ARG("string", "format");
    ARG("int", "flags");
    DOC_FUNC("Parameter `flags` is an enum of type UI_SliderFlags");

    SFUN(ui_SliderInt, "int", "slider");
    ARG("string", "label");
    ARG("UI_Int", "v");
    ARG("int", "v_min");
    ARG("int", "v_max");
    DOC_FUNC("Implied format = \"%d\", flags = 0");

    SFUN(ui_SliderIntEx, "int", "slider");
    ARG("string", "label");
    ARG("UI_Int", "v");
    ARG("int", "v_min");
    ARG("int", "v_max");
    ARG("string", "format");
    ARG("int", "flags");
    DOC_FUNC("Parameter `flags` is an enum of type UI_SliderFlags");

    SFUN(ui_SliderScalarN_CKINT, "int", "slider");
    ARG("string", "label");
    ARG("int[]", "data");
    ARG("int", "v_min");
    ARG("int", "v_max");

    SFUN(ui_SliderScalarNEx_CKINT, "int", "slider");
    ARG("string", "label");
    ARG("int[]", "data");
    ARG("int", "v_min");
    ARG("int", "v_max");
    ARG("string", "format");
    ARG("int", "flags");
    DOC_FUNC("Parameter `flags` is an enum of type UI_SliderFlags");

    SFUN(ui_SliderScalarN_CKFLOAT, "int", "slider");
    ARG("string", "label");
    ARG("float[]", "data");
    ARG("float", "v_min");
    ARG("float", "v_max");

    SFUN(ui_SliderScalarNEx_CKFLOAT, "int", "slider");
    ARG("string", "label");
    ARG("float[]", "data");
    ARG("float", "v_min");
    ARG("float", "v_max");
    ARG("string", "format");
    ARG("int", "flags");
    DOC_FUNC("Parameter `flags` is an enum of type UI_SliderFlags");

    SFUN(ui_VSliderFloat, "int", "vslider");
    ARG("string", "label");
    ARG("vec2", "size");
    ARG("UI_Float", "v");
    ARG("float", "v_min");
    ARG("float", "v_max");
    DOC_FUNC("Implied format = \"%.3f\", flags = 0");

    SFUN(ui_VSliderFloatEx, "int", "vslider");
    ARG("string", "label");
    ARG("vec2", "size");
    ARG("UI_Float", "v");
    ARG("float", "v_min");
    ARG("float", "v_max");
    ARG("string", "format");
    ARG("int", "flags");
    DOC_FUNC("Parameter `flags` is an enum of type UI_SliderFlags");

    SFUN(ui_VSliderInt, "int", "vslider");
    ARG("string", "label");
    ARG("vec2", "size");
    ARG("UI_Int", "v");
    ARG("int", "v_min");
    ARG("int", "v_max");
    DOC_FUNC("Implied format = \"%d\", flags = 0");

    SFUN(ui_VSliderIntEx, "int", "vslider");
    ARG("string", "label");
    ARG("vec2", "size");
    ARG("UI_Int", "v");
    ARG("int", "v_min");
    ARG("int", "v_max");
    ARG("string", "format");
    ARG("int", "flags");
    DOC_FUNC("Parameter `flags` is an enum of type UI_SliderFlags");

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

// UI_Int -------------------------------------------------------------------
CK_DLL_CTOR(ui_int_ctor)
{
    int* i                                   = new int(0);
    OBJ_MEMBER_UINT(SELF, ui_int_ptr_offset) = (t_CKUINT)i;
}

CK_DLL_DTOR(ui_int_dtor)
{
    int* i = (int*)OBJ_MEMBER_UINT(SELF, ui_int_ptr_offset);
    delete i;
    OBJ_MEMBER_UINT(SELF, ui_int_ptr_offset) = 0;
}

CK_DLL_MFUN(ui_int_get_value)
{
    int* i        = (int*)OBJ_MEMBER_UINT(SELF, ui_int_ptr_offset);
    RETURN->v_int = *i;
}

CK_DLL_MFUN(ui_int_set_value)
{
    int* i        = (int*)OBJ_MEMBER_UINT(SELF, ui_int_ptr_offset);
    *i            = GET_NEXT_INT(ARGS);
    RETURN->v_int = *i;
}

// UI_Int2 -------------------------------------------------------------------

CK_DLL_CTOR(ui_int2_ctor)
{
    int* i                                    = new int[2]{ 0, 0 };
    OBJ_MEMBER_UINT(SELF, ui_int2_ptr_offset) = (t_CKUINT)i;
}

CK_DLL_DTOR(ui_int2_dtor)
{
    int* i = (int*)OBJ_MEMBER_UINT(SELF, ui_int2_ptr_offset);
    delete[] i;
    OBJ_MEMBER_UINT(SELF, ui_int2_ptr_offset) = 0;
}

CK_DLL_MFUN(ui_int2_get_x)
{
    int* i        = (int*)OBJ_MEMBER_UINT(SELF, ui_int2_ptr_offset);
    RETURN->v_int = i[0];
}

CK_DLL_MFUN(ui_int2_get_y)
{
    int* i        = (int*)OBJ_MEMBER_UINT(SELF, ui_int2_ptr_offset);
    RETURN->v_int = i[1];
}

CK_DLL_MFUN(ui_int2_set)
{
    int* i = (int*)OBJ_MEMBER_UINT(SELF, ui_int2_ptr_offset);
    i[0]   = GET_NEXT_INT(ARGS);
    i[1]   = GET_NEXT_INT(ARGS);
}

// UI_Int3 -------------------------------------------------------------------

CK_DLL_CTOR(ui_int3_ctor)
{
    int* i                                    = new int[3]{ 0, 0, 0 };
    OBJ_MEMBER_UINT(SELF, ui_int3_ptr_offset) = (t_CKUINT)i;
}

CK_DLL_DTOR(ui_int3_dtor)
{
    int* i = (int*)OBJ_MEMBER_UINT(SELF, ui_int3_ptr_offset);
    delete[] i;
    OBJ_MEMBER_UINT(SELF, ui_int3_ptr_offset) = 0;
}

CK_DLL_MFUN(ui_int3_get_x)
{
    int* i        = (int*)OBJ_MEMBER_UINT(SELF, ui_int3_ptr_offset);
    RETURN->v_int = i[0];
}

CK_DLL_MFUN(ui_int3_get_y)
{
    int* i        = (int*)OBJ_MEMBER_UINT(SELF, ui_int3_ptr_offset);
    RETURN->v_int = i[1];
}

CK_DLL_MFUN(ui_int3_get_z)
{
    int* i        = (int*)OBJ_MEMBER_UINT(SELF, ui_int3_ptr_offset);
    RETURN->v_int = i[2];
}

CK_DLL_MFUN(ui_int3_set)
{
    int* i = (int*)OBJ_MEMBER_UINT(SELF, ui_int3_ptr_offset);
    i[0]   = GET_NEXT_INT(ARGS);
    i[1]   = GET_NEXT_INT(ARGS);
    i[2]   = GET_NEXT_INT(ARGS);
}

// UI_Int4 -------------------------------------------------------------------

CK_DLL_CTOR(ui_int4_ctor)
{
    int* i                                    = new int[4]{ 0, 0, 0, 0 };
    OBJ_MEMBER_UINT(SELF, ui_int4_ptr_offset) = (t_CKUINT)i;
}

CK_DLL_DTOR(ui_int4_dtor)
{
    int* i = (int*)OBJ_MEMBER_UINT(SELF, ui_int4_ptr_offset);
    delete[] i;
    OBJ_MEMBER_UINT(SELF, ui_int4_ptr_offset) = 0;
}

CK_DLL_MFUN(ui_int4_get_x)
{
    int* i        = (int*)OBJ_MEMBER_UINT(SELF, ui_int4_ptr_offset);
    RETURN->v_int = i[0];
}

CK_DLL_MFUN(ui_int4_get_y)
{
    int* i        = (int*)OBJ_MEMBER_UINT(SELF, ui_int4_ptr_offset);
    RETURN->v_int = i[1];
}

CK_DLL_MFUN(ui_int4_get_z)
{
    int* i        = (int*)OBJ_MEMBER_UINT(SELF, ui_int4_ptr_offset);
    RETURN->v_int = i[2];
}

CK_DLL_MFUN(ui_int4_get_w)
{
    int* i        = (int*)OBJ_MEMBER_UINT(SELF, ui_int4_ptr_offset);
    RETURN->v_int = i[3];
}

CK_DLL_MFUN(ui_int4_set)
{
    int* i = (int*)OBJ_MEMBER_UINT(SELF, ui_int4_ptr_offset);
    i[0]   = GET_NEXT_INT(ARGS);
    i[1]   = GET_NEXT_INT(ARGS);
    i[2]   = GET_NEXT_INT(ARGS);
    i[3]   = GET_NEXT_INT(ARGS);
}

// UI_Float -------------------------------------------------------------------
CK_DLL_CTOR(ui_float_ctor)
{
    float* f                                   = new float(0.0f);
    OBJ_MEMBER_UINT(SELF, ui_float_ptr_offset) = (t_CKUINT)f;
}

CK_DLL_DTOR(ui_float_dtor)
{
    float* f = (float*)OBJ_MEMBER_UINT(SELF, ui_float_ptr_offset);
    delete f;
    OBJ_MEMBER_UINT(SELF, ui_float_ptr_offset) = 0;
}

CK_DLL_MFUN(ui_float_get_value)
{
    float* f        = (float*)OBJ_MEMBER_UINT(SELF, ui_float_ptr_offset);
    RETURN->v_float = *f;
}

CK_DLL_MFUN(ui_float_set_value)
{
    float* f        = (float*)OBJ_MEMBER_UINT(SELF, ui_float_ptr_offset);
    *f              = GET_NEXT_FLOAT(ARGS);
    RETURN->v_float = *f;
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

// UI_Float2 -----------------------------------------------------------------

CK_DLL_CTOR(ui_float2_ctor)
{
    float* f                                    = new float[2]{ 0.0f, 0.0f };
    OBJ_MEMBER_UINT(SELF, ui_float2_ptr_offset) = (t_CKUINT)f;
}

CK_DLL_DTOR(ui_float2_dtor)
{
    float* f = (float*)OBJ_MEMBER_UINT(SELF, ui_float2_ptr_offset);
    delete[] f;
    OBJ_MEMBER_UINT(SELF, ui_float2_ptr_offset) = 0;
}

CK_DLL_MFUN(ui_float2_get_value)
{
    float* f       = (float*)OBJ_MEMBER_UINT(SELF, ui_float2_ptr_offset);
    RETURN->v_vec2 = { f[0], f[1] };
}

CK_DLL_MFUN(ui_float2_set_value)
{
    float* f       = (float*)OBJ_MEMBER_UINT(SELF, ui_float2_ptr_offset);
    t_CKVEC2 v     = GET_NEXT_VEC2(ARGS);
    f[0]           = v.x;
    f[1]           = v.y;
    RETURN->v_vec2 = v;
}

// UI_Float3 -----------------------------------------------------------------

CK_DLL_CTOR(ui_float3_ctor)
{
    float* f = new float[3]{ 0.0f, 0.0f, 0.0f };
    OBJ_MEMBER_UINT(SELF, ui_float3_ptr_offset) = (t_CKUINT)f;
}

CK_DLL_DTOR(ui_float3_dtor)
{
    float* f = (float*)OBJ_MEMBER_UINT(SELF, ui_float3_ptr_offset);
    delete[] f;
    OBJ_MEMBER_UINT(SELF, ui_float3_ptr_offset) = 0;
}

CK_DLL_MFUN(ui_float3_get_value)
{
    float* f       = (float*)OBJ_MEMBER_UINT(SELF, ui_float3_ptr_offset);
    RETURN->v_vec3 = { f[0], f[1], f[2] };
}

CK_DLL_MFUN(ui_float3_set_value)
{
    float* f       = (float*)OBJ_MEMBER_UINT(SELF, ui_float3_ptr_offset);
    t_CKVEC3 v     = GET_NEXT_VEC3(ARGS);
    f[0]           = v.x;
    f[1]           = v.y;
    f[2]           = v.z;
    RETURN->v_vec3 = v;
}

// UI_Float4 -----------------------------------------------------------------

CK_DLL_CTOR(ui_float4_ctor)
{
    float* f = new float[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
    OBJ_MEMBER_UINT(SELF, ui_float4_ptr_offset) = (t_CKUINT)f;
}

CK_DLL_DTOR(ui_float4_dtor)
{
    float* f = (float*)OBJ_MEMBER_UINT(SELF, ui_float4_ptr_offset);
    delete[] f;
    OBJ_MEMBER_UINT(SELF, ui_float4_ptr_offset) = 0;
}

CK_DLL_MFUN(ui_float4_get_value)
{
    float* f       = (float*)OBJ_MEMBER_UINT(SELF, ui_float4_ptr_offset);
    RETURN->v_vec4 = { f[0], f[1], f[2], f[3] };
}

CK_DLL_MFUN(ui_float4_set_value)
{
    float* f       = (float*)OBJ_MEMBER_UINT(SELF, ui_float4_ptr_offset);
    t_CKVEC4 v     = GET_NEXT_VEC4(ARGS);
    f[0]           = v.x;
    f[1]           = v.y;
    f[2]           = v.z;
    f[3]           = v.w;
    RETURN->v_vec4 = v;
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
// Content region
// ============================================================================

CK_DLL_SFUN(ui_GetContentRegionAvail)
{
    cimgui::ImVec2 avail = cimgui::ImGui_GetContentRegionAvail();
    RETURN->v_vec2       = { avail.x, avail.y };
}

CK_DLL_SFUN(ui_GetContentRegionMax)
{
    cimgui::ImVec2 max = cimgui::ImGui_GetContentRegionMax();
    RETURN->v_vec2     = { max.x, max.y };
}
CK_DLL_SFUN(ui_GetWindowContentRegionMin)
{
    cimgui::ImVec2 min = cimgui::ImGui_GetWindowContentRegionMin();
    RETURN->v_vec2     = { min.x, min.y };
}

CK_DLL_SFUN(ui_GetWindowContentRegionMax)
{
    cimgui::ImVec2 max = cimgui::ImGui_GetWindowContentRegionMax();
    RETURN->v_vec2     = { max.x, max.y };
}

// ============================================================================
// Windows Scrolling
// ============================================================================

CK_DLL_SFUN(ui_GetScrollX)
{
    RETURN->v_float = cimgui::ImGui_GetScrollX();
}

CK_DLL_SFUN(ui_GetScrollY)
{
    RETURN->v_float = cimgui::ImGui_GetScrollY();
}

CK_DLL_SFUN(ui_SetScrollX)
{
    cimgui::ImGui_SetScrollX((float)GET_NEXT_FLOAT(ARGS));
}

CK_DLL_SFUN(ui_SetScrollY)
{
    cimgui::ImGui_SetScrollY((float)GET_NEXT_FLOAT(ARGS));
}

CK_DLL_SFUN(ui_GetScrollMaxX)
{
    RETURN->v_float = cimgui::ImGui_GetScrollMaxX();
}

CK_DLL_SFUN(ui_GetScrollMaxY)
{
    RETURN->v_float = cimgui::ImGui_GetScrollMaxY();
}

CK_DLL_SFUN(ui_SetScrollHereX)
{
    cimgui::ImGui_SetScrollHereX((float)GET_NEXT_FLOAT(ARGS));
}

CK_DLL_SFUN(ui_SetScrollHereY)
{
    cimgui::ImGui_SetScrollHereY((float)GET_NEXT_FLOAT(ARGS));
}

CK_DLL_SFUN(ui_SetScrollFromPosX)
{
    float local_x        = (float)GET_NEXT_FLOAT(ARGS);
    float center_x_ratio = (float)GET_NEXT_FLOAT(ARGS);
    cimgui::ImGui_SetScrollFromPosX(local_x, center_x_ratio);
}

CK_DLL_SFUN(ui_SetScrollFromPosY)
{
    float local_y        = (float)GET_NEXT_FLOAT(ARGS);
    float center_y_ratio = (float)GET_NEXT_FLOAT(ARGS);
    cimgui::ImGui_SetScrollFromPosY(local_y, center_y_ratio);
}

// ============================================================================
// Parameters stacks (shared)
// ============================================================================

CK_DLL_SFUN(ui_PushStyleColorImVec4)
{
    int idx        = GET_NEXT_INT(ARGS);
    t_CKVEC4 color = GET_NEXT_VEC4(ARGS);
    cimgui::ImGui_PushStyleColorImVec4(
      idx, { (float)color.x, (float)color.y, (float)color.z, (float)color.w });
}
CK_DLL_SFUN(ui_PopStyleColor)
{
    cimgui::ImGui_PopStyleColor();
}
CK_DLL_SFUN(ui_PopStyleColorEx)
{
    cimgui::ImGui_PopStyleColorEx(GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(ui_PushStyleVar)
{
    int idx   = GET_NEXT_INT(ARGS);
    float val = GET_NEXT_FLOAT(ARGS);
    cimgui::ImGui_PushStyleVar(idx, val);
}

CK_DLL_SFUN(ui_PushStyleVarImVec2)
{
    int idx      = GET_NEXT_INT(ARGS);
    t_CKVEC2 val = GET_NEXT_VEC2(ARGS);
    cimgui::ImGui_PushStyleVarImVec2(idx, { (float)val.x, (float)val.y });
}

CK_DLL_SFUN(ui_PopStyleVar)
{
    cimgui::ImGui_PopStyleVar();
}

CK_DLL_SFUN(ui_PopStyleVarEx)
{
    cimgui::ImGui_PopStyleVarEx(GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(ui_PushTabStop)
{
    cimgui::ImGui_PushTabStop(GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(ui_PopTabStop)
{
    cimgui::ImGui_PopTabStop();
}

CK_DLL_SFUN(ui_PushButtonRepeat)
{
    cimgui::ImGui_PushButtonRepeat(GET_NEXT_INT(ARGS));
}
CK_DLL_SFUN(ui_PopButtonRepeat)
{
    cimgui::ImGui_PopButtonRepeat();
}

// ============================================================================
// Style read access
// ============================================================================
CK_DLL_SFUN(ui_GetFontSize)
{
    RETURN->v_float = cimgui::ImGui_GetFontSize();
}

CK_DLL_SFUN(ui_GetFontTexUvWhitePixel)
{
    cimgui::ImVec2 uv = cimgui::ImGui_GetFontTexUvWhitePixel();
    RETURN->v_vec2    = { uv.x, uv.y };
}

CK_DLL_SFUN(ui_GetColorU32)
{
    RETURN->v_int = cimgui::ImGui_GetColorU32(GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(ui_GetColorU32Ex)
{
    t_CKINT idx     = GET_NEXT_INT(ARGS);
    t_CKFLOAT alpha = GET_NEXT_FLOAT(ARGS);
    RETURN->v_int   = cimgui::ImGui_GetColorU32Ex(idx, (float)alpha);
}

CK_DLL_SFUN(ui_GetColorU32ImVec4)
{
    t_CKVEC4 col  = GET_NEXT_VEC4(ARGS);
    RETURN->v_int = cimgui::ImGui_GetColorU32ImVec4(
      { (float)col.x, (float)col.y, (float)col.z, (float)col.w });
}

CK_DLL_SFUN(ui_GetStyleColorVec4)
{
    t_CKINT idx               = GET_NEXT_INT(ARGS);
    const cimgui::ImVec4* col = cimgui::ImGui_GetStyleColorVec4(idx);
    RETURN->v_vec4            = { col->x, col->y, col->z, col->w };
}

// ============================================================================
// Layout cursor positioning
// ============================================================================
CK_DLL_SFUN(ui_GetCursorScreenPos)
{
    cimgui::ImVec2 pos = cimgui::ImGui_GetCursorScreenPos();
    RETURN->v_vec2     = { pos.x, pos.y };
}

CK_DLL_SFUN(ui_SetCursorScreenPos)
{
    t_CKVEC2 pos = GET_NEXT_VEC2(ARGS);
    cimgui::ImGui_SetCursorScreenPos({ (float)pos.x, (float)pos.y });
}

CK_DLL_SFUN(ui_GetCursorPos)
{
    cimgui::ImVec2 pos = cimgui::ImGui_GetCursorPos();
    RETURN->v_vec2     = { pos.x, pos.y };
}

CK_DLL_SFUN(ui_GetCursorPosX)
{
    RETURN->v_int = cimgui::ImGui_GetCursorPosX();
}

CK_DLL_SFUN(ui_GetCursorPosY)
{
    RETURN->v_int = cimgui::ImGui_GetCursorPosY();
}

CK_DLL_SFUN(ui_SetCursorPos)
{
    t_CKVEC2 local_pos = GET_NEXT_VEC2(ARGS);
    cimgui::ImGui_SetCursorPos({ (float)local_pos.x, (float)local_pos.y });
}

CK_DLL_SFUN(ui_SetCursorPosX)
{
    cimgui::ImGui_SetCursorPosX((float)GET_NEXT_FLOAT(ARGS));
}

CK_DLL_SFUN(ui_SetCursorPosY)
{
    cimgui::ImGui_SetCursorPosY((float)GET_NEXT_FLOAT(ARGS));
}

CK_DLL_SFUN(ui_GetCursorStartPos)
{
    cimgui::ImVec2 pos = cimgui::ImGui_GetCursorStartPos();
    RETURN->v_vec2     = { pos.x, pos.y };
}

// ============================================================================
// Other layout functions
// ============================================================================
CK_DLL_SFUN(ui_Separator)
{
    cimgui::ImGui_Separator();
}

CK_DLL_SFUN(ui_SameLine)
{
    cimgui::ImGui_SameLine();
}

CK_DLL_SFUN(ui_SameLineEx)
{
    float offset_from_start_x = (float)GET_NEXT_FLOAT(ARGS);
    float spacing             = (float)GET_NEXT_FLOAT(ARGS);
    cimgui::ImGui_SameLineEx(offset_from_start_x, spacing);
}

CK_DLL_SFUN(ui_NewLine)
{
    cimgui::ImGui_NewLine();
}

CK_DLL_SFUN(ui_Spacing)
{
    cimgui::ImGui_Spacing();
}

CK_DLL_SFUN(ui_Dummy)
{
    t_CKVEC2 size = GET_NEXT_VEC2(ARGS);
    cimgui::ImGui_Dummy({ (float)size.x, (float)size.y });
}

CK_DLL_SFUN(ui_Indent)
{
    cimgui::ImGui_Indent();
}

CK_DLL_SFUN(ui_IndentEx)
{
    cimgui::ImGui_IndentEx((float)GET_NEXT_FLOAT(ARGS));
}

CK_DLL_SFUN(ui_Unindent)
{
    cimgui::ImGui_Unindent();
}

CK_DLL_SFUN(ui_UnindentEx)
{
    cimgui::ImGui_UnindentEx((float)GET_NEXT_FLOAT(ARGS));
}

CK_DLL_SFUN(ui_BeginGroup)
{
    cimgui::ImGui_BeginGroup();
}

CK_DLL_SFUN(ui_EndGroup)
{
    cimgui::ImGui_EndGroup();
}

CK_DLL_SFUN(ui_AlignTextToFramePadding)
{
    cimgui::ImGui_AlignTextToFramePadding();
}

CK_DLL_SFUN(ui_GetTextLineHeight)
{
    RETURN->v_float = cimgui::ImGui_GetTextLineHeight();
}

CK_DLL_SFUN(ui_GetTextLineHeightWithSpacing)
{
    RETURN->v_float = cimgui::ImGui_GetTextLineHeightWithSpacing();
}

CK_DLL_SFUN(ui_GetFrameHeight)
{
    RETURN->v_float = cimgui::ImGui_GetFrameHeight();
}

CK_DLL_SFUN(ui_GetFrameHeightWithSpacing)
{
    RETURN->v_float = cimgui::ImGui_GetFrameHeightWithSpacing();
}

// ============================================================================
// ID stack/scopes
// ============================================================================

CK_DLL_SFUN(ui_PushID)
{
    Chuck_String* ck_string = GET_NEXT_STRING(ARGS);
    cimgui::ImGui_PushID(API->object->str(ck_string));
}

CK_DLL_SFUN(ui_PushIDStr)
{
    Chuck_String* ck_string_begin = GET_NEXT_STRING(ARGS);
    Chuck_String* ck_string_end   = GET_NEXT_STRING(ARGS);
    cimgui::ImGui_PushIDStr(API->object->str(ck_string_begin),
                            API->object->str(ck_string_end));
}

CK_DLL_SFUN(ui_PushIDInt)
{
    cimgui::ImGui_PushIDInt(GET_NEXT_INT(ARGS));
}

CK_DLL_SFUN(ui_PopID)
{
    cimgui::ImGui_PopID();
}

CK_DLL_SFUN(ui_GetID)
{
    Chuck_String* ck_string = GET_NEXT_STRING(ARGS);
    RETURN->v_int           = cimgui::ImGui_GetID(API->object->str(ck_string));
}

CK_DLL_SFUN(ui_GetIDStr)
{
    Chuck_String* ck_string_begin = GET_NEXT_STRING(ARGS);
    Chuck_String* ck_string_end   = GET_NEXT_STRING(ARGS);
    RETURN->v_int = cimgui::ImGui_GetIDStr(API->object->str(ck_string_begin),
                                           API->object->str(ck_string_end));
}

// ============================================================================
// Widgets: Text
// ============================================================================

CK_DLL_SFUN(ui_TextUnformatted)
{
    const char* text = API->object->str(GET_NEXT_STRING(ARGS));
    cimgui::ImGui_TextUnformatted(text);
}

CK_DLL_SFUN(ui_TextUnformattedEx)
{
    const char* text     = API->object->str(GET_NEXT_STRING(ARGS));
    const char* text_end = API->object->str(GET_NEXT_STRING(ARGS));
    cimgui::ImGui_TextUnformattedEx(text, text_end);
}

CK_DLL_SFUN(ui_TextColoredUnformatted)
{
    t_CKVEC4 col     = GET_NEXT_VEC4(ARGS);
    const char* text = API->object->str(GET_NEXT_STRING(ARGS));
    cimgui::ImGui_TextColoredUnformatted(
      { (float)col.x, (float)col.y, (float)col.z, (float)col.w }, text);
}

CK_DLL_SFUN(ui_TextDisabledUnformatted)
{
    const char* text = API->object->str(GET_NEXT_STRING(ARGS));
    cimgui::ImGui_TextDisabledUnformatted(text);
}

CK_DLL_SFUN(ui_TextWrappedUnformatted)
{
    const char* text = API->object->str(GET_NEXT_STRING(ARGS));
    cimgui::ImGui_TextWrappedUnformatted(text);
}

CK_DLL_SFUN(ui_LabelTextUnformatted)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));
    const char* text  = API->object->str(GET_NEXT_STRING(ARGS));
    cimgui::ImGui_LabelTextUnformatted(label, text);
}

CK_DLL_SFUN(ui_BulletTextUnformatted)
{
    const char* text = API->object->str(GET_NEXT_STRING(ARGS));
    cimgui::ImGui_BulletTextUnformatted(text);
}

CK_DLL_SFUN(ui_SeparatorText)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));
    cimgui::ImGui_SeparatorText(label);
}

// ============================================================================
// Callbacks
// ============================================================================

CK_DLL_CTOR(ui_callback_ctor)
{
    // origin shred needed for API->vm->invoke_mfun_immediate_mode
    chugin_setOriginShred(SELF, SHRED);
}

CK_DLL_DTOR(ui_callback_dtor)
{
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

// user defined callback
CK_DLL_MFUN(ui_combo_callback)
{
}

// ============================================================================
// Widgets: Main
// ============================================================================

CK_DLL_SFUN(ui_Button)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));
    RETURN->v_int     = cimgui::ImGui_Button(label);
}

CK_DLL_SFUN(ui_ButtonEx)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));
    t_CKVEC2 size     = GET_NEXT_VEC2(ARGS);
    RETURN->v_int
      = cimgui::ImGui_ButtonEx(label, { (float)size.x, (float)size.y });
}

CK_DLL_SFUN(ui_SmallButton)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));
    RETURN->v_int     = cimgui::ImGui_SmallButton(label);
}

CK_DLL_SFUN(ui_InvisibleButton)
{
    const char* str_id = API->object->str(GET_NEXT_STRING(ARGS));
    t_CKVEC2 size      = GET_NEXT_VEC2(ARGS);
    int flags          = GET_NEXT_INT(ARGS);
    RETURN->v_int      = cimgui::ImGui_InvisibleButton(
      str_id, { (float)size.x, (float)size.y }, flags);
}

CK_DLL_SFUN(ui_ArrowButton)
{
    const char* str_id = API->object->str(GET_NEXT_STRING(ARGS));
    int direction      = GET_NEXT_INT(ARGS);
    RETURN->v_int      = cimgui::ImGui_ArrowButton(str_id, direction);
}

CK_DLL_SFUN(ui_Checkbox)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));

    Chuck_Object* obj = GET_NEXT_OBJECT(ARGS);
    bool* b           = (bool*)OBJ_MEMBER_UINT(obj, ui_bool_ptr_offset);

    RETURN->v_int = cimgui::ImGui_Checkbox(label, b);
}

CK_DLL_SFUN(ui_CheckboxFlagsIntPtr)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));

    Chuck_Object* obj = GET_NEXT_OBJECT(ARGS);
    int* flags        = (int*)OBJ_MEMBER_UINT(obj, ui_int_ptr_offset);

    int flags_value = GET_NEXT_INT(ARGS);

    RETURN->v_int
      = cimgui::ImGui_CheckboxFlagsIntPtr(label, flags, flags_value);
}

CK_DLL_SFUN(ui_RadioButton)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));
    int active        = GET_NEXT_INT(ARGS);
    RETURN->v_int     = cimgui::ImGui_RadioButton(label, active);
}

CK_DLL_SFUN(ui_RadioButtonIntPtr)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));

    Chuck_Object* obj = GET_NEXT_OBJECT(ARGS);
    int* v            = (int*)OBJ_MEMBER_UINT(obj, ui_int_ptr_offset);

    int v_button = GET_NEXT_INT(ARGS);

    RETURN->v_int = cimgui::ImGui_RadioButtonIntPtr(label, v, v_button);
}

CK_DLL_SFUN(ui_ProgressBar)
{
    float fraction = GET_NEXT_FLOAT(ARGS);
    t_CKVEC2 size  = GET_NEXT_VEC2(ARGS);
    const char* overlay
      = API->object->str(GET_NEXT_STRING(ARGS)); // NULL = no overlay
    cimgui::ImGui_ProgressBar(fraction, { (float)size.x, (float)size.y },
                              overlay);
}

CK_DLL_SFUN(ui_Bullet)
{
    cimgui::ImGui_Bullet();
}

// ============================================================================
// Widgets: Combo Box (Dropdown)
// ============================================================================
CK_DLL_SFUN(ui_BeginCombo)
{
    const char* label         = API->object->str(GET_NEXT_STRING(ARGS));
    const char* preview_value = API->object->str(GET_NEXT_STRING(ARGS));
    int flags                 = GET_NEXT_INT(ARGS);
    RETURN->v_int = cimgui::ImGui_BeginCombo(label, preview_value, flags);
}

CK_DLL_SFUN(ui_EndCombo)
{
    cimgui::ImGui_EndCombo();
}

CK_DLL_SFUN(ui_ComboChar)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));

    Chuck_Object* obj = GET_NEXT_OBJECT(ARGS);
    int* current_item = (int*)OBJ_MEMBER_UINT(obj, ui_int_ptr_offset);

    Chuck_ArrayInt* ck_string_array = (Chuck_ArrayInt*)GET_NEXT_OBJECT(ARGS);
    int num_items = API->object->array_int_size(ck_string_array);

    // copy array of chuck_string to array of const char*
    const char** items
      = ARENA_PUSH_COUNT(&audio_frame_arena, const char*, num_items);

    for (int i = 0; i < num_items; i++) {
        items[i] = API->object->str(
          (Chuck_String*)API->object->array_int_get_idx(ck_string_array, i));
    }

    RETURN->v_int
      = cimgui::ImGui_ComboChar(label, current_item, items, num_items);
}

CK_DLL_SFUN(ui_ComboCharEx)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));

    Chuck_Object* obj = GET_NEXT_OBJECT(ARGS);
    int* current_item = (int*)OBJ_MEMBER_UINT(obj, ui_int_ptr_offset);

    Chuck_ArrayInt* ck_string_array = (Chuck_ArrayInt*)GET_NEXT_OBJECT(ARGS);
    int num_items = API->object->array_int_size(ck_string_array);

    int popup_max_height_in_items = GET_NEXT_INT(ARGS);

    // copy array of chuck_string to array of const char*
    const char** items
      = ARENA_PUSH_COUNT(&audio_frame_arena, const char*, num_items);

    for (int i = 0; i < num_items; i++) {
        items[i] = API->object->str(
          (Chuck_String*)API->object->array_int_get_idx(ck_string_array, i));
    }

    RETURN->v_int = cimgui::ImGui_ComboCharEx(
      label, current_item, items, num_items, popup_max_height_in_items);
}

CK_DLL_SFUN(ui_Combo)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));

    Chuck_Object* obj = GET_NEXT_OBJECT(ARGS);
    int* current_item = (int*)OBJ_MEMBER_UINT(obj, ui_int_ptr_offset);

    const char* items_separated_by_zeros
      = API->object->str(GET_NEXT_STRING(ARGS));
    printf("items_separated_by_zeros: %s\n", items_separated_by_zeros);

    RETURN->v_int
      = cimgui::ImGui_Combo(label, current_item, items_separated_by_zeros);
}

CK_DLL_SFUN(ui_ComboEx)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));

    Chuck_Object* obj = GET_NEXT_OBJECT(ARGS);
    int* current_item = (int*)OBJ_MEMBER_UINT(obj, ui_int_ptr_offset);

    const char* items_separated_by_zeros
      = API->object->str(GET_NEXT_STRING(ARGS));

    int popup_max_height_in_items = GET_NEXT_INT(ARGS);

    RETURN->v_int = cimgui::ImGui_ComboEx(
      label, current_item, items_separated_by_zeros, popup_max_height_in_items);
}

// ============================================================================
// Widgets: Drag Sliders
// ============================================================================

#define UI_DRAG_EX_IMPL_FLOAT(func, type, ptr_offset)                          \
    {                                                                          \
        const char* label  = API->object->str(GET_NEXT_STRING(ARGS));          \
        Chuck_Object* obj  = GET_NEXT_OBJECT(ARGS);                            \
        type* v            = (type*)OBJ_MEMBER_UINT(obj, ptr_offset);          \
        float v_speed      = GET_NEXT_FLOAT(ARGS);                             \
        float v_min        = GET_NEXT_FLOAT(ARGS);                             \
        float v_max        = GET_NEXT_FLOAT(ARGS);                             \
        const char* format = API->object->str(GET_NEXT_STRING(ARGS));          \
        int flags          = GET_NEXT_INT(ARGS);                               \
        RETURN->v_int                                                          \
          = cimgui::func(label, v, v_speed, v_min, v_max, format, flags);      \
    }

#define UI_DRAG_EX_IMPL_INT(func, type, ptr_offset)                            \
    {                                                                          \
        const char* label  = API->object->str(GET_NEXT_STRING(ARGS));          \
        Chuck_Object* obj  = GET_NEXT_OBJECT(ARGS);                            \
        type* v            = (type*)OBJ_MEMBER_UINT(obj, ptr_offset);          \
        float v_speed      = GET_NEXT_FLOAT(ARGS);                             \
        int v_min          = GET_NEXT_INT(ARGS);                               \
        int v_max          = GET_NEXT_INT(ARGS);                               \
        const char* format = API->object->str(GET_NEXT_STRING(ARGS));          \
        int flags          = GET_NEXT_INT(ARGS);                               \
        RETURN->v_int                                                          \
          = cimgui::func(label, v, v_speed, v_min, v_max, format, flags);      \
    }

#define UI_DRAG_IMPL(func, type, ptr_offset)                                   \
    {                                                                          \
        const char* label = API->object->str(GET_NEXT_STRING(ARGS));           \
        Chuck_Object* obj = GET_NEXT_OBJECT(ARGS);                             \
        type* v           = (type*)OBJ_MEMBER_UINT(obj, ptr_offset);           \
        RETURN->v_int     = cimgui::func(label, v);                            \
    }

CK_DLL_SFUN(ui_DragFloat)
{
    UI_DRAG_IMPL(ImGui_DragFloat, float, ui_float_ptr_offset);
}

CK_DLL_SFUN(ui_DragFloatEx)
{
    UI_DRAG_EX_IMPL_FLOAT(ImGui_DragFloatEx, float, ui_float_ptr_offset);
}

CK_DLL_SFUN(ui_DragFloat2)
{
    UI_DRAG_IMPL(ImGui_DragFloat2, float, ui_float2_ptr_offset);
}

CK_DLL_SFUN(ui_DragFloat2Ex)
{
    UI_DRAG_EX_IMPL_FLOAT(ImGui_DragFloat2Ex, float, ui_float2_ptr_offset);
}

CK_DLL_SFUN(ui_DragFloat3)
{
    UI_DRAG_IMPL(ImGui_DragFloat3, float, ui_float3_ptr_offset);
}

CK_DLL_SFUN(ui_DragFloat3Ex)
{
    UI_DRAG_EX_IMPL_FLOAT(ImGui_DragFloat3Ex, float, ui_float3_ptr_offset);
}

CK_DLL_SFUN(ui_DragFloat4)
{
    UI_DRAG_IMPL(ImGui_DragFloat4, float, ui_float4_ptr_offset);
}

CK_DLL_SFUN(ui_DragFloat4Ex)
{
    UI_DRAG_EX_IMPL_FLOAT(ImGui_DragFloat4Ex, float, ui_float4_ptr_offset);
}

CK_DLL_SFUN(ui_DragFloatRange2)
{
    const char* label     = API->object->str(GET_NEXT_STRING(ARGS));
    Chuck_Object* obj_min = GET_NEXT_OBJECT(ARGS);
    float* v_current_min
      = (float*)OBJ_MEMBER_UINT(obj_min, ui_float2_ptr_offset);
    Chuck_Object* obj_max = GET_NEXT_OBJECT(ARGS);
    float* v_current_max
      = (float*)OBJ_MEMBER_UINT(obj_max, ui_float2_ptr_offset);

    RETURN->v_int
      = cimgui::ImGui_DragFloatRange2(label, v_current_min, v_current_max);
}

CK_DLL_SFUN(ui_DragFloatRange2Ex)
{
    const char* label     = API->object->str(GET_NEXT_STRING(ARGS));
    Chuck_Object* obj_min = GET_NEXT_OBJECT(ARGS);
    float* v_current_min
      = (float*)OBJ_MEMBER_UINT(obj_min, ui_float2_ptr_offset);
    Chuck_Object* obj_max = GET_NEXT_OBJECT(ARGS);
    float* v_current_max
      = (float*)OBJ_MEMBER_UINT(obj_max, ui_float2_ptr_offset);
    float v_speed      = GET_NEXT_FLOAT(ARGS);
    float v_min        = GET_NEXT_FLOAT(ARGS);
    float v_max        = GET_NEXT_FLOAT(ARGS);
    const char* format = API->object->str(GET_NEXT_STRING(ARGS));
    const char* format_max
      = API->object->str(GET_NEXT_STRING(ARGS)); // NULL = same format as min
    float power   = GET_NEXT_FLOAT(ARGS);
    RETURN->v_int = cimgui::ImGui_DragFloatRange2Ex(
      label, v_current_min, v_current_max, v_speed, v_min, v_max, format,
      format_max, power);
}

CK_DLL_SFUN(ui_DragInt)
{
    UI_DRAG_IMPL(ImGui_DragInt, int, ui_int_ptr_offset);
}

CK_DLL_SFUN(ui_DragIntEx)
{
    UI_DRAG_EX_IMPL_INT(ImGui_DragIntEx, int, ui_int_ptr_offset);
}

CK_DLL_SFUN(ui_DragInt2)
{
    UI_DRAG_IMPL(ImGui_DragInt2, int, ui_int2_ptr_offset);
}

CK_DLL_SFUN(ui_DragInt2Ex)
{
    UI_DRAG_EX_IMPL_INT(ImGui_DragInt2Ex, int, ui_int2_ptr_offset);
}

CK_DLL_SFUN(ui_DragInt3)
{
    UI_DRAG_IMPL(ImGui_DragInt3, int, ui_int3_ptr_offset);
}

CK_DLL_SFUN(ui_DragInt3Ex)
{
    UI_DRAG_EX_IMPL_INT(ImGui_DragInt3Ex, int, ui_int3_ptr_offset);
}

CK_DLL_SFUN(ui_DragInt4)
{
    UI_DRAG_IMPL(ImGui_DragInt4, int, ui_int4_ptr_offset);
}

CK_DLL_SFUN(ui_DragInt4Ex)
{
    UI_DRAG_EX_IMPL_INT(ImGui_DragInt4Ex, int, ui_int4_ptr_offset);
}

CK_DLL_SFUN(ui_DragIntRange2)
{
    const char* label     = API->object->str(GET_NEXT_STRING(ARGS));
    Chuck_Object* obj_min = GET_NEXT_OBJECT(ARGS);
    int* v_current_min    = (int*)OBJ_MEMBER_UINT(obj_min, ui_int2_ptr_offset);
    Chuck_Object* obj_max = GET_NEXT_OBJECT(ARGS);
    int* v_current_max    = (int*)OBJ_MEMBER_UINT(obj_max, ui_int2_ptr_offset);

    RETURN->v_int
      = cimgui::ImGui_DragIntRange2(label, v_current_min, v_current_max);
}

CK_DLL_SFUN(ui_DragIntRange2Ex)
{
    const char* label     = API->object->str(GET_NEXT_STRING(ARGS));
    Chuck_Object* obj_min = GET_NEXT_OBJECT(ARGS);
    int* v_current_min    = (int*)OBJ_MEMBER_UINT(obj_min, ui_int2_ptr_offset);
    Chuck_Object* obj_max = GET_NEXT_OBJECT(ARGS);
    int* v_current_max    = (int*)OBJ_MEMBER_UINT(obj_max, ui_int2_ptr_offset);
    float v_speed         = GET_NEXT_FLOAT(ARGS);
    int v_min             = GET_NEXT_INT(ARGS);
    int v_max             = GET_NEXT_INT(ARGS);
    const char* format    = API->object->str(GET_NEXT_STRING(ARGS));
    const char* format_max
      = API->object->str(GET_NEXT_STRING(ARGS)); // NULL = same format as min
    int flags     = GET_NEXT_INT(ARGS);
    RETURN->v_int = cimgui::ImGui_DragIntRange2Ex(
      label, v_current_min, v_current_max, v_speed, v_min, v_max, format,
      format_max, flags);
}

CK_DLL_SFUN(ui_DragScalarN_CKINT)
{
    const char* label        = API->object->str(GET_NEXT_STRING(ARGS));
    Chuck_ArrayInt* ck_array = (Chuck_ArrayInt*)GET_NEXT_OBJECT(ARGS);

    int num_components = API->object->array_int_size(ck_array);

    int* v = ARENA_PUSH_COUNT(&audio_frame_arena, int, num_components);

    for (int i = 0; i < num_components; ++i) {
        v[i] = API->object->array_int_get_idx(ck_array, i);
    }

    RETURN->v_int = cimgui::ImGui_DragScalarN(label, cimgui::ImGuiDataType_S32,
                                              v, num_components);

    API->object->array_int_clear(ck_array);

    // copy back
    for (int i = 0; i < num_components; ++i) {
        API->object->array_int_push_back(ck_array, v[i]);
    }
}

CK_DLL_SFUN(ui_DragScalarNEx_CKINT)
{
    const char* label        = API->object->str(GET_NEXT_STRING(ARGS));
    Chuck_ArrayInt* ck_array = (Chuck_ArrayInt*)GET_NEXT_OBJECT(ARGS);

    int num_components = API->object->array_int_size(ck_array);

    int* v = ARENA_PUSH_COUNT(&audio_frame_arena, int, num_components);

    for (int i = 0; i < num_components; ++i) {
        v[i] = API->object->array_int_get_idx(ck_array, i);
    }

    float speed        = GET_NEXT_FLOAT(ARGS);
    int v_min          = GET_NEXT_INT(ARGS);
    int v_max          = GET_NEXT_INT(ARGS);
    const char* format = API->object->str(GET_NEXT_STRING(ARGS));
    int flags          = GET_NEXT_INT(ARGS);

    RETURN->v_int = cimgui::ImGui_DragScalarNEx(
      label, cimgui::ImGuiDataType_S32, v, num_components, speed, &v_min,
      &v_max, format, flags);

    // copy back
    API->object->array_int_clear(ck_array);
    for (int i = 0; i < num_components; ++i) {
        API->object->array_int_push_back(ck_array, v[i]);
    }
}

CK_DLL_SFUN(ui_DragScalarN_CKFLOAT)
{
    const char* label          = API->object->str(GET_NEXT_STRING(ARGS));
    Chuck_ArrayFloat* ck_array = (Chuck_ArrayFloat*)GET_NEXT_OBJECT(ARGS);

    int num_components = API->object->array_float_size(ck_array);

    float* v = ARENA_PUSH_COUNT(&audio_frame_arena, float, num_components);

    for (int i = 0; i < num_components; ++i) {
        v[i] = API->object->array_float_get_idx(ck_array, i);
    }

    RETURN->v_int = cimgui::ImGui_DragScalarN(
      label, cimgui::ImGuiDataType_Float, v, num_components);

    API->object->array_float_clear(ck_array);

    // copy back
    for (int i = 0; i < num_components; ++i) {
        API->object->array_float_push_back(ck_array, v[i]);
    }
}

CK_DLL_SFUN(ui_DragScalarNEx_CKFLOAT)
{
    const char* label          = API->object->str(GET_NEXT_STRING(ARGS));
    Chuck_ArrayFloat* ck_array = (Chuck_ArrayFloat*)GET_NEXT_OBJECT(ARGS);

    int num_components = API->object->array_float_size(ck_array);

    // copy chuck array to arena
    float* v = ARENA_PUSH_COUNT(&audio_frame_arena, float, num_components);
    for (int i = 0; i < num_components; ++i) {
        v[i] = API->object->array_float_get_idx(ck_array, i);
    }

    // get other params
    float speed        = GET_NEXT_FLOAT(ARGS);
    float v_min        = GET_NEXT_FLOAT(ARGS);
    float v_max        = GET_NEXT_FLOAT(ARGS);
    const char* format = API->object->str(GET_NEXT_STRING(ARGS));
    int flags          = GET_NEXT_INT(ARGS);

    RETURN->v_int = cimgui::ImGui_DragScalarNEx(
      label, cimgui::ImGuiDataType_Float, v, num_components, speed, &v_min,
      &v_max, format, flags);

    // copy back to chuck array (no set in API, have to clear and re-push)
    API->object->array_float_clear(ck_array);
    for (int i = 0; i < num_components; ++i) {
        API->object->array_float_push_back(ck_array, v[i]);
    }
}

// ============================================================================
// Widgets: Slider
// ============================================================================

CK_DLL_SFUN(ui_SliderFloat)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));

    Chuck_Object* obj = GET_NEXT_OBJECT(ARGS);
    float* v          = (float*)OBJ_MEMBER_UINT(obj, ui_float_ptr_offset);

    float v_min = GET_NEXT_FLOAT(ARGS);
    float v_max = GET_NEXT_FLOAT(ARGS);

    RETURN->v_int = cimgui::ImGui_SliderFloat(label, v, v_min, v_max);
}

CK_DLL_SFUN(ui_SliderFloatEx)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));

    Chuck_Object* obj = GET_NEXT_OBJECT(ARGS);
    float* v          = (float*)OBJ_MEMBER_UINT(obj, ui_float_ptr_offset);

    float v_min = GET_NEXT_FLOAT(ARGS);
    float v_max = GET_NEXT_FLOAT(ARGS);
    const char* format
      = API->object->str(GET_NEXT_STRING(ARGS)); // NULL = "%.3f"
    int flags = GET_NEXT_INT(ARGS);

    RETURN->v_int
      = cimgui::ImGui_SliderFloatEx(label, v, v_min, v_max, format, flags);
}

CK_DLL_SFUN(ui_SliderAngle)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));

    Chuck_Object* obj = GET_NEXT_OBJECT(ARGS);
    float* v          = (float*)OBJ_MEMBER_UINT(obj, ui_float_ptr_offset);

    RETURN->v_int = cimgui::ImGui_SliderAngle(label, v);
}

CK_DLL_SFUN(ui_SliderAngleEx)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));

    Chuck_Object* obj = GET_NEXT_OBJECT(ARGS);
    float* v          = (float*)OBJ_MEMBER_UINT(obj, ui_float_ptr_offset);

    float v_degrees_min = GET_NEXT_FLOAT(ARGS);
    float v_degrees_max = GET_NEXT_FLOAT(ARGS);

    const char* format
      = API->object->str(GET_NEXT_STRING(ARGS)); // NULL = "%.3f"

    int flags = GET_NEXT_INT(ARGS);

    RETURN->v_int = cimgui::ImGui_SliderAngleEx(label, v, v_degrees_min,
                                                v_degrees_max, format, flags);
}

CK_DLL_SFUN(ui_SliderInt)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));

    Chuck_Object* obj = GET_NEXT_OBJECT(ARGS);
    int* v            = (int*)OBJ_MEMBER_UINT(obj, ui_int_ptr_offset);

    int v_min = GET_NEXT_INT(ARGS);
    int v_max = GET_NEXT_INT(ARGS);

    RETURN->v_int = cimgui::ImGui_SliderInt(label, v, v_min, v_max);
}

CK_DLL_SFUN(ui_SliderIntEx)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));

    Chuck_Object* obj = GET_NEXT_OBJECT(ARGS);
    int* v            = (int*)OBJ_MEMBER_UINT(obj, ui_int_ptr_offset);

    int v_min = GET_NEXT_INT(ARGS);
    int v_max = GET_NEXT_INT(ARGS);
    const char* format
      = API->object->str(GET_NEXT_STRING(ARGS)); // NULL = "%.0f"
    int flags = GET_NEXT_INT(ARGS);

    RETURN->v_int
      = cimgui::ImGui_SliderIntEx(label, v, v_min, v_max, format, flags);
}

CK_DLL_SFUN(ui_SliderScalarN_CKINT)
{
    const char* label        = API->object->str(GET_NEXT_STRING(ARGS));
    Chuck_ArrayInt* ck_array = (Chuck_ArrayInt*)GET_NEXT_OBJECT(ARGS);

    int num_components = API->object->array_int_size(ck_array);

    int* v = ARENA_PUSH_COUNT(&audio_frame_arena, int, num_components);

    for (int i = 0; i < num_components; ++i) {
        v[i] = API->object->array_int_get_idx(ck_array, i);
    }

    int v_min = GET_NEXT_INT(ARGS);
    int v_max = GET_NEXT_INT(ARGS);

    RETURN->v_int = cimgui::ImGui_SliderScalarN(
      label, cimgui::ImGuiDataType_S32, v, num_components, &v_min, &v_max);

    API->object->array_int_clear(ck_array);

    // copy back
    for (int i = 0; i < num_components; ++i) {
        API->object->array_int_push_back(ck_array, v[i]);
    }
}

CK_DLL_SFUN(ui_SliderScalarNEx_CKINT)
{
    const char* label        = API->object->str(GET_NEXT_STRING(ARGS));
    Chuck_ArrayInt* ck_array = (Chuck_ArrayInt*)GET_NEXT_OBJECT(ARGS);

    int num_components = API->object->array_int_size(ck_array);

    int* v = ARENA_PUSH_COUNT(&audio_frame_arena, int, num_components);

    for (int i = 0; i < num_components; ++i) {
        v[i] = API->object->array_int_get_idx(ck_array, i);
    }

    int v_min = GET_NEXT_INT(ARGS);
    int v_max = GET_NEXT_INT(ARGS);
    const char* format
      = API->object->str(GET_NEXT_STRING(ARGS)); // NULL = "%.0f"
    int flags = GET_NEXT_INT(ARGS);

    RETURN->v_int = cimgui::ImGui_SliderScalarNEx(
      label, cimgui::ImGuiDataType_S32, v, num_components, &v_min, &v_max,
      format, flags);

    // copy back
    API->object->array_int_clear(ck_array);
    for (int i = 0; i < num_components; ++i) {
        API->object->array_int_push_back(ck_array, v[i]);
    }
}

CK_DLL_SFUN(ui_SliderScalarN_CKFLOAT)
{
    const char* label          = API->object->str(GET_NEXT_STRING(ARGS));
    Chuck_ArrayFloat* ck_array = (Chuck_ArrayFloat*)GET_NEXT_OBJECT(ARGS);

    int num_components = API->object->array_float_size(ck_array);

    float* v = ARENA_PUSH_COUNT(&audio_frame_arena, float, num_components);

    for (int i = 0; i < num_components; ++i) {
        v[i] = API->object->array_float_get_idx(ck_array, i);
    }

    float v_min = GET_NEXT_FLOAT(ARGS);
    float v_max = GET_NEXT_FLOAT(ARGS);

    RETURN->v_int = cimgui::ImGui_SliderScalarN(
      label, cimgui::ImGuiDataType_Float, v, num_components, &v_min, &v_max);

    API->object->array_float_clear(ck_array);

    // copy back
    for (int i = 0; i < num_components; ++i) {
        API->object->array_float_push_back(ck_array, v[i]);
    }
}

CK_DLL_SFUN(ui_SliderScalarNEx_CKFLOAT)
{
    const char* label          = API->object->str(GET_NEXT_STRING(ARGS));
    Chuck_ArrayFloat* ck_array = (Chuck_ArrayFloat*)GET_NEXT_OBJECT(ARGS);

    int num_components = API->object->array_float_size(ck_array);

    float* v = ARENA_PUSH_COUNT(&audio_frame_arena, float, num_components);

    for (int i = 0; i < num_components; ++i) {
        v[i] = API->object->array_float_get_idx(ck_array, i);
    }

    float v_min = GET_NEXT_FLOAT(ARGS);
    float v_max = GET_NEXT_FLOAT(ARGS);
    const char* format
      = API->object->str(GET_NEXT_STRING(ARGS)); // NULL = "%.0f"
    int flags = GET_NEXT_INT(ARGS);

    RETURN->v_int = cimgui::ImGui_SliderScalarNEx(
      label, cimgui::ImGuiDataType_Float, v, num_components, &v_min, &v_max,
      format, flags);

    // copy back
    API->object->array_float_clear(ck_array);
    for (int i = 0; i < num_components; ++i) {
        API->object->array_float_push_back(ck_array, v[i]);
    }
}

CK_DLL_SFUN(ui_VSliderFloat)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));
    t_CKVEC2 size     = GET_NEXT_VEC2(ARGS);

    Chuck_Object* obj = GET_NEXT_OBJECT(ARGS);
    float* v          = (float*)OBJ_MEMBER_UINT(obj, ui_float_ptr_offset);

    float v_min = GET_NEXT_FLOAT(ARGS);
    float v_max = GET_NEXT_FLOAT(ARGS);

    RETURN->v_int = cimgui::ImGui_VSliderFloat(
      label, { (float)size.x, (float)size.y }, v, v_min, v_max);
}

CK_DLL_SFUN(ui_VSliderFloatEx)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));
    t_CKVEC2 size     = GET_NEXT_VEC2(ARGS);

    Chuck_Object* obj = GET_NEXT_OBJECT(ARGS);
    float* v          = (float*)OBJ_MEMBER_UINT(obj, ui_float_ptr_offset);

    float v_min = GET_NEXT_FLOAT(ARGS);
    float v_max = GET_NEXT_FLOAT(ARGS);
    const char* format
      = API->object->str(GET_NEXT_STRING(ARGS)); // NULL = "%.3f"
    int flags = GET_NEXT_INT(ARGS);

    RETURN->v_int = cimgui::ImGui_VSliderFloatEx(
      label, { (float)size.x, (float)size.y }, v, v_min, v_max, format, flags);
}

CK_DLL_SFUN(ui_VSliderInt)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));
    t_CKVEC2 size     = GET_NEXT_VEC2(ARGS);

    Chuck_Object* obj = GET_NEXT_OBJECT(ARGS);
    int* v            = (int*)OBJ_MEMBER_UINT(obj, ui_int_ptr_offset);

    int v_min = GET_NEXT_INT(ARGS);
    int v_max = GET_NEXT_INT(ARGS);

    RETURN->v_int = cimgui::ImGui_VSliderInt(
      label, { (float)size.x, (float)size.y }, v, v_min, v_max);
}

CK_DLL_SFUN(ui_VSliderIntEx)
{
    const char* label = API->object->str(GET_NEXT_STRING(ARGS));
    t_CKVEC2 size     = GET_NEXT_VEC2(ARGS);

    Chuck_Object* obj = GET_NEXT_OBJECT(ARGS);
    int* v            = (int*)OBJ_MEMBER_UINT(obj, ui_int_ptr_offset);

    int v_min = GET_NEXT_INT(ARGS);
    int v_max = GET_NEXT_INT(ARGS);
    const char* format
      = API->object->str(GET_NEXT_STRING(ARGS)); // NULL = "%.0f"
    int flags = GET_NEXT_INT(ARGS);

    RETURN->v_int = cimgui::ImGui_VSliderIntEx(
      label, { (float)size.x, (float)size.y }, v, v_min, v_max, format, flags);
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