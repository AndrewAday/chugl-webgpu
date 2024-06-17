UI_Bool main_window;
UI_Bool demo_window;
UI_Bool metrics_window;
true => main_window.val;
true => demo_window.val;
true => metrics_window.val;

UI_Int combo_char_current_item;
["hello", "this", "is", "a", "test"] @=> string combo_char_item_arr[];

UI_Int2 drag_int2;
UI_Int vslider_int;

UI_String input_string;
"hello chugl" => input_string.val;
UI_String input_string_multi;

UI_Float input_float;

UI_Float3 color_edit;
UI_Float4 color_picker;


[1, 2, 3, 4, 5] @=> int input_int_arr[];
[1.0, 2.0, 3.0, 4.0, 5.0] @=> float input_float_arr[];

[1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 5.0, 2.0] @=> float line_plot_arr[];
[1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 5.0, 2.0] @=> float histogram_plot_arr[];

class SizeCallback extends UI_SizeCallback {
    fun void handler(UI_SizeCallbackData data) {
        // <<< "pos:", data.getPos() >>>;
        // <<< "current size:", data.currentSize() >>>;
        // <<< "desired size:", data.desiredSize() >>>;
        // <<< now >>>;
    }
} SizeCallback sizeCallback;

[0, 1, 2, 3, 4, 5] @=> int drag_arr[];
[0.1, 0.2, 0.3, 0.4, 0.5] @=> float drag_arr_f[];

UI_Bool selectable_bool;

UI_Int listbox_int;
["hello", "this", "is", "a", "test"] @=> string listbox_item_arr[];

// <<< UI_Key.Backslash >>>;
0 => int fc;

fun void drawUI() {
while (1) {

    GG.nextFrame() => now;

    // if (fc == 0) UI.styleColorsLight();

    if (main_window.val()) {
        UI.setNextWindowSizeConstraints(
            @(1, 1), // size_min
            @(1000, 1000), // size_max
            sizeCallback
        );
        if (UI.begin("ChuGL UI Window", main_window, UI_WindowFlags.None)) {
            // UI.pushButtonRepeat(true);
            if (UI.button("say dope!")) {
                <<< "dope!" >>>;
            }

            if (
                UI.combo("comboChar", combo_char_current_item, combo_char_item_arr)
            ) {
                <<< "comboChar:", combo_char_item_arr[combo_char_current_item.val()], combo_char_current_item.val() >>>;
            }

            if (UI.drag("drag int2", drag_int2, 10.0, 0, 100, null, 0)) {
                <<< "drag int2:", drag_int2.x(), drag_int2.y() >>>;
            }

            if (UI.drag("drag scalar N ints", drag_arr)) {
                <<< "drag scalar N ints:" >>>;
                for (auto i : drag_arr) <<< i >>>;
            }

            if (UI.drag(
                "drag scalar N floats", drag_arr_f, 0.01, 
                0.1, 1.0, "%.2f", 0
                )
            ) {
                <<< "drag scalar N floats:" >>>;
                for (auto i : drag_arr_f) <<< i >>>;
            }

            if (UI.vslider("vslider", @(20, 100), vslider_int, 0, 100)) {
                <<< "vslider:", vslider_int.val() >>>;
            }

            if (UI.inputText("input string", input_string, 12, 0)) {
                <<< "input string:", input_string.val() >>>;
            }

            if (UI.inputTextMultiline("input multi", input_string_multi)) {
                <<< "input string:", input_string_multi.val() >>>;
            }

            if (UI.inputFloat("input float", input_float)) {
                <<< "input float:", input_float.val() >>>;
            }

            if (UI.inputFloat("input float", input_float_arr)) {
                <<< "input float arr:" >>>;
                for (auto i : input_float_arr) <<< i >>>;
            }

            if (UI.inputInt("input int", input_int_arr)) {
                <<< "input int arr:" >>>;
                for (auto i : input_int_arr) <<< i >>>;
            }

            if (UI.colorButton("what", @(0.0, 1.0, 0.0, 1.0), UI_ColorEditFlags.None, @(100, 100))) {
                <<< "color button" >>>;
            }

            if (UI.colorEdit("color edit", color_edit, 0)) {
                color_edit.val() => vec3 color_edit_val;
                <<< "color edit:", color_edit_val >>>;
            }

                if (UI.colorPicker("color picker", color_picker, 0, @(1.0, 0.0, 0.0, 0.0))) {
                color_picker.val() => vec4 color_picker_val;
                <<< "color picker:", color_picker_val >>>;
            }

            if (UI.treeNode("tree node")) {
                UI.text("tree node text");
                UI.treePop();
            }

            if (UI.selectable("selectable", selectable_bool, UI_SelectableFlags.None)) {
                <<< "selectable" >>>;
            }

            if (UI.listBox("listbox", listbox_int, listbox_item_arr, -1)) {
                <<< "listbox:", listbox_item_arr[listbox_int.val()] >>>;
            }

            UI.plotLines("line plot", line_plot_arr);

            UI.plotHistogram("histogram plot", histogram_plot_arr);

            // doesn't work
            // if ( 
            //     UI.combo("combo", combo_char_current_item, "one\0two\0three\0\0")
            // ) {
            //     <<< "combo separated by zeros:", combo_char_current_item.val() >>>;
            // }
            // UI.popButtonRepeat();

            // <<< UI.clipboardText() >>>;
        }
        UI.showStyleEditor();

        UI.end();
    }

    // if (demo_window.val()) {
    //     UI.showDemoWindow(demo_window);
    // } 

    // if (metrics_window.val()) {
    //     UI.showMetricsWindow(metrics_window);
    // }
    fc++;
}
}

spork ~ drawUI();
1::eon => now;