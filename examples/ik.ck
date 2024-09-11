// http://www.andreasaristidou.com/FABRIK.html

SphereGeometry sphere_geo;
FlatMaterial flat_material;

40 => int NUM_LENGTHS;
float lengths[NUM_LENGTHS];
for (int i; i < NUM_LENGTHS; i++) {
    4.0 / NUM_LENGTHS => lengths[i];
}
vec3 points[lengths.size() + 1]; // N points

GMesh joints[0];
for (int i; i < points.size(); i++) {
    GMesh joint(sphere_geo, flat_material) --> GG.scene();
    joints << joint;
    .1 => joint.sca;
}

GCamera camera --> GG.scene();
camera.posZ(5);
camera.orthographic();
GG.scene().camera(camera); // set main camera

// fun void mouse() {
//     while (true) {
//         GG.nextFrame() => now;
//         if (UI.isMouseClicked(UI_MouseButton.Left)) { // TODO track mouse click in GWindow
//             <<< "Mouse position: ", GWindow.mousePos() >>>;
//             camera.screenCoordToWorldPos(GWindow.mousePos(), 5) => vec3 pos;

//             // spawn a sphere at the intersection point
//             GMesh sphere(sphere_geo, flat_material) --> GG.scene();
//             .1 => sphere.sca;
//             pos => sphere.pos;

//             <<< "Screen position: ", camera.worldPosToScreenCoord(pos) >>>;
//         }
//     }
// }
// spork ~ mouse();

fun void fabrik(vec3 start_target, vec3 end_target, vec3 points[], float lengths[]) {
    0.01 => float TOLERANCE;
    10 => int MAX_ITERATIONS;
    0 => float total_length;
    for (0 => int i; i < lengths.size(); i++) {
        lengths[i] +=> total_length;
    }
    // case: end target out of reach
    end_target - start_target => vec3 dir;
    if (total_length < dir.magnitude()) {
        dir.normalize();
        // set all points to point towards end_target
        for (1 => int i; i < points.size(); i++) {
            dir * lengths[i - 1] + points[i - 1] => points[i];
        }
        return;
    }

    points.size() => int N;
    repeat (MAX_ITERATIONS) {
        if (Math.euclidean(points[points.size() - 1], end_target) < TOLERANCE) {
            return;
        }

        // forward
        end_target => points[N-1]; // put last point at target
        for (N - 2 => int i; i >= 0; i--) {
            points[i] - points[i+1] => vec3 dir;
            dir.normalize();
            points[i + 1] + dir * lengths[i] => points[i];
        }

        // backwards
        start_target => points[0];
        for (1 => int i; i < points.size() - 1; i++) {
            points[i] - points[i-1] => vec3 dir; 
            dir.normalize();
            lengths[i-1] * dir + points[i-1] => points[i];
        }
    }
}


fun void draw() {
    camera.screenCoordToWorldPos(GWindow.mousePos(), 5) => vec3 end_target;
    0 => end_target.z; // put on plane
    fabrik(@(0, 0, 0), end_target, points, lengths);

    for (int i; i < points.size(); i++) {
        points[i] => joints[i].pos;
    }
}

while (true) {
    GG.nextFrame() => now;
    draw();
}