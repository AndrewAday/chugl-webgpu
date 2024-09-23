GPoints points --> GG.scene();

points.positions([@(-1, -1, 0), @(1, -1, 0), @(0, 1, 0)]);

while (true) {
    GG.nextFrame() => now;
}