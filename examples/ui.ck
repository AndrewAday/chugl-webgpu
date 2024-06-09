UI_Bool bool;

true => bool.val;

while (1) {
    GG.nextFrame() => now;
    UI.begin();
    if (UI.button("say dope!")) {
        <<< "dope!" >>>;
    }
    UI.end();
}