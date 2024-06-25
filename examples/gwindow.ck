/*
gwindow.ck
Window management demo.

Press '1' to go fullscreen.
Press '2' to go windowed.
Press '3' to maximize (windowed fullscreen).

Author: Andrew Zhu Aday (azaday) June 2024
*/

GWindow.closeable(false);

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


while (1) { 
    GG.nextFrame() => now;
    if (UI.isKeyPressed(UI_Key.Num1)) GWindow.fullscreen();
    if (UI.isKeyPressed(UI_Key.Num2)) GWindow.windowed(800, 600);
    if (UI.isKeyPressed(UI_Key.Num3)) GWindow.maximize();
}
