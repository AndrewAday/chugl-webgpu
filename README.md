# chugl-webgpu


Build for XCode:
```
cmake -B build-xcode -G Xcode -DCMAKE_BUILD_TYPE=Debug -DCHUGL_BUILD_RENDERER_TESTS=ON
```

Build for Web

```
emcmake cmake -B build-web
```

## Directories

**src/chugl-tests**: chugl integration tests. All tests are chuck programs exercising the chugin interface. A test passes if nothing is printed to stderr or stdout.
- to run tests: `chuck T.ck tester.ck`
  - `T.ck` is the test harness/framework
  - `tester.ck` is the test runner

## ChucK / ChuGin improvements

- QUERY interface should take type as an enum like `CK_INT` rather than a string like `"int"`
  - Also unclear what the size of the types is... is this a 32bit or 64bit int? Need to know to prevent memory issues. 
E.g. this would cause a bug if chuck expected an i64, not i32. (when reading wrap_repeat it would also pull in the bytes of wrap_mirror)
```c
        static i32 WRAP_REPEAT = SG_SAMPLER_WRAP_REPEAT;
        static i32 WRAP_MIRROR = SG_SAMPLER_WRAP_MIRROR_REPEAT;
        QUERY->add_svar(QUERY, "int", "WRAP_REPEAT", true, &WRAP_REPEAT);
        QUERY->add_svar(QUERY, "int", "WRAP_REPEAT", true, &WRAP_REPEAT);
```

- chugin needs to support `f32` floats. halves memory pressure for all chugl graphics data movement
  - also makes a lot of library binding easier, don't have to copy doubles --> floats for many operations

- `  void (CK_DLL_CALL * const callback_on_instantiate)( f_callback_on_instantiate callback, Type base_type, Chuck_VM * vm, t_CKBOOL shouldSetShredOrigin );`
  - why use this over DLL_CTOR?
- why do we need to use origin_shred (ie most ancestor shred) to `invoke_mfun_immediate_mode` on a chuck_object method?
  - prevents us from invoking update() on default GGens created during the original Query, such as mainScene and mainCamera

### improvements for ImGui bindings
- A better way to pass pointers to data (maybe pass by `ref`?)
- default function parameters
- function pointers! removes need to wrap callback handlers with a custom class with vtable base handler() function, and override that function. See `UI_SizeCallback` chugin type for example
- supported '\0' null character in chuck strings gotten from API->object->str()

- Better chugin array API
  - array_set_idx. would greatly improve efficiency of Imgui_SetScalarN impl for drag sliders
    - currently to modify array, we have to clear *Everything* and re-push the new values. wasteful when only 1 element is changed in a frame, as in a drag slider
  - way to get readonly const* to contiguous array data (so that we don't have to copy into arena)
    - this also requires a floats / ints rather than long / double
