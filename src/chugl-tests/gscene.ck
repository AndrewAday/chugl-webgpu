GCamera camera --> GScene scene;

T.assert(scene.camera() == null, "default camera null");

scene.camera(camera);
T.assert(scene.camera().id() == camera.id(), "scene set camera");
