/*
gwindow.ck
Window management demo.

Press '1' to go fullscreen.
Press '2' to go windowed.
Press '3' to maximize (windowed fullscreen).
Press '4' to iconify / restore
press '5' to toggle window opacity
press '6' to toggle mouse mode (normal - hidden - disabled)
press '7' to swtich between custom a


Author: Andrew Zhu Aday (azaday) June 2024
*/

GWindow.closeable(false);
// GWindow.sizeLimits(100, 100, 1920, 1080, @(16, 9));  // uncomment to set size limits
GWindow.windowed(1200, 675);
GWindow.center(); // center window on screen
GWindow.title("GWindow Demo");

// window configuration (must be called BEFORE first GG.nextFrame())
true => GWindow.transparent;
true => GWindow.floating;
// false => GWindow.decorated;
// false => GWindow.resizable;


fun void closeCallback() {
    while (1) {
        GWindow.closeEvent() => now;
        <<< "user tried to close window" >>>;
        GWindow.close(); // allow close 
    }
} spork ~ closeCallback();

fun void resizeCallback() {
    while (1) {
        GWindow.resizeEvent() => now;
        <<< "user resized window | window size: ", GWindow.windowSize(), " | framebuffer size: ", GWindow.framebufferSize() >>>;
    }
} spork ~ resizeCallback();

fun void contentScaleCallback() {
    while (1) {
        GWindow.contentScaleEvent() => now;
        <<< "content scale changed | content scale: ", GWindow.contentScale() >>>;
    }
} spork ~ contentScaleCallback();

fun void makeCustomCursor() {
    // make cursor into a greem dot
    int cursor_image[8 * 8 * 4];
    for (int i; i < 8 * 8 * 4; 4 +=> i) {
        0 => cursor_image[i];
        255 => cursor_image[i + 1];
        0 => cursor_image[i + 2];
        255 => cursor_image[i + 3];
    }
    GWindow.customCursor(cursor_image, 8, 8, 0, 0);
}

int fc;
true => int opaque;
false => int floating;
0 => int mouse_mode;
false => int custom_cursor;
while (1) { 
    GG.nextFrame() => now;
    if (UI.isKeyPressed(UI_Key.Num1)) GWindow.fullscreen();
    if (UI.isKeyPressed(UI_Key.Num2)) {
        GWindow.windowed(800, 600);
        GWindow.center();
    }
    if (UI.isKeyPressed(UI_Key.Num3)) {
        // disable size limits
        GWindow.sizeLimits(0, 0, 0, 0, @(0, 0));
        GWindow.maximize();
    }
    if (UI.isKeyPressed(UI_Key.Num4)) {
        GWindow.iconify();
        repeat (60) {
            GG.nextFrame() => now;
        }
        GWindow.restore();
    }
    if (UI.isKeyPressed(UI_Key.Num5)) {
        if (opaque) {
            GWindow.opacity(0.4);
            false => opaque;
        } else {
            GWindow.opacity(1.0);
            true => opaque;
        }
    }
    if (UI.isKeyPressed(UI_Key.Num6, false)) {
        (mouse_mode + 1) % 3 => mouse_mode;
        if (mouse_mode == 0) {
            <<< "mouse mode: normal" >>>;
        } else if (mouse_mode == 1) {
            <<< "mouse mode: hidden" >>>;
        } else {
            <<< "mouse mode: disabled" >>>;
        }
        GWindow.mouseMode(mouse_mode);
    }
    if (UI.isKeyPressed(UI_Key.Num7, false)) {
        if (custom_cursor) {
            <<< "switching to normal cursor" >>>;
            GWindow.normalCursor();
            false => custom_cursor;
        } else {
            <<< "switching to custom cursor" >>>;
            makeCustomCursor();
            true => custom_cursor;
        }
    }

    // <<< "Mouse Pos", GWindow.mousePos(), "Mouse Deltas", GWindow.mouseDeltaPos() >>>;

    GWindow.title("GWindow Demo | Frame: " + (++fc));
}
