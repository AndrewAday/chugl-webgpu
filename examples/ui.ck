UI_Bool main_window;
UI_Bool demo_window;
UI_Bool metrics_window;
true => main_window.val;
true => demo_window.val;
true => metrics_window.val;

class SizeCallback extends UI_SizeCallback {
    fun void handler(UI_SizeCallbackData data) {
        <<< "pos:", data.getPos() >>>;
        <<< "current size:", data.currentSize() >>>;
        <<< "desired size:", data.desiredSize() >>>;
        <<< now >>>;
    }
} SizeCallback sizeCallback;

// <<< UI_Key.Backslash >>>;
0 => int fc;

fun void drawUI() {
while (1) {

    GG.nextFrame() => now;

    if (fc == 0) UI.styleColorsLight();

    if (main_window.val()) {
        <<< "here" >>>;
        UI.setNextWindowSizeConstraints(
            @(1, 1), // size_min
            @(100, 100), // size_max
            sizeCallback
        );
        if (UI.begin("ChuGL UI Window", main_window, UI_WindowFlags.None)) {
            if (UI.button("say dope!")) {
                <<< "dope!" >>>;
            }
        }
        // UI.showStyleEditor();

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