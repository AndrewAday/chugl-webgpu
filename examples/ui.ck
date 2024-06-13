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

            // doesn't work
            // if ( 
            //     UI.combo("combo", combo_char_current_item, "one\0two\0three\0\0")
            // ) {
            //     <<< "combo separated by zeros:", combo_char_current_item.val() >>>;
            // }
            // UI.popButtonRepeat();
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