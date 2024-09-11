GCamera camera --> GScene scene;

T.assert(scene.camera() == null, "default camera null");

scene.camera(camera);
T.assert(scene.camera().id() == camera.id(), "scene set camera");

T.assert(T.veq(scene.backgroundColor(), @(0, 0, 0, 1)), "default background color");
scene.backgroundColor(@(0.1, 0.2, 0.3, 0.4));
T.assert(T.veq(scene.backgroundColor(), @(0.1, 0.2, 0.3, 0.4)), "set background color");

// TODO test ambient light

