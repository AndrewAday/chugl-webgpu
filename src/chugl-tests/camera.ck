GCamera camera;

T.assert(GCamera.PERSPECTIVE == 0, "GCamera.PERSPECTIVE");
T.assert(GCamera.ORTHOGRAPHIC == 1, "GCamera.ORTHOGRAPHIC");

T.assert(camera.mode() == GCamera.PERSPECTIVE, "default camera mode");
camera.orthographic();
T.assert(camera.mode() == GCamera.ORTHOGRAPHIC, "camera mode orthographic");
camera.perspective();
T.assert(camera.mode() == GCamera.PERSPECTIVE, "camera mode perspective");

camera.clip(0.3, 333.0);
T.assert(T.feq(camera.clipNear(), 0.3), "camera clip near");
T.assert(T.feq(camera.clipFar(), 333.0), "camera clip far");

T.assert(T.feq(camera.fov(), Math.PI / 4.0), "default camera fov");

camera.fov(Math.PI);
T.assert(T.feq(camera.fov(), Math.PI), "camera fov");

camera.size(5.0);
T.assert(T.feq(camera.size(), 5.0), "camera size");
