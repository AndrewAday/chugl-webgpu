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

// mouse picking / ray casting
camera.perspective();
@(123, 456) => vec2 mouse_pos;  
@(.5, .4, .3) => vec3 ndc;
camera.screenCoordToWorldPos(@(123, 456), 20) => vec3 pos;
camera.worldPosToScreenCoord(pos) => vec2 screen_pos;
camera.NDCToWorldPos(ndc) => vec3 ndc_to_world_pos;
T.assert(T.veq(screen_pos, mouse_pos), "persp camera screenCoordToWorldPos and worldPosToScreenCoord");
T.assert(T.veq(camera.worldPosToNDC(ndc_to_world_pos), ndc), "persp camera NDCToWorldPos");

camera.orthographic();
camera.screenCoordToWorldPos(@(123, 456), 20) => pos;
camera.worldPosToScreenCoord(pos) => screen_pos;
camera.NDCToWorldPos(ndc) => ndc_to_world_pos;
T.assert(T.veq(screen_pos, mouse_pos), "ortho camera screenCoordToWorldPos and worldPosToScreenCoord");
T.assert(T.veq(camera.worldPosToNDC(ndc_to_world_pos), ndc), "ortho camera NDCToWorldPos"); 


// OrbitCamera ========================================

OrbitCamera orbit_camera;

orbit_camera.dragSpeed(0.123);
T.assert(T.feq(orbit_camera.dragSpeed(), 0.123), "orbit camera drag speed");

orbit_camera.zoomSpeed(0.456);
T.assert(T.feq(orbit_camera.zoomSpeed(), 0.456), "orbit camera zoom speed");

// FlyCamera =========================================


