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
E.g. this would case a bug if chuck expected an i64, not i32. (when reading wrap_repeat it would also pull in the bytes of wrap_mirror)
```c
        static i32 WRAP_REPEAT = SG_SAMPLER_WRAP_REPEAT;
        static i32 WRAP_MIRROR = SG_SAMPLER_WRAP_MIRROR_REPEAT;
        QUERY->add_svar(QUERY, "int", "WRAP_REPEAT", true, &WRAP_REPEAT);
        QUERY->add_svar(QUERY, "int", "WRAP_REPEAT", true, &WRAP_REPEAT);
```

- chugin needs to support `f32` floats. halves memory pressure for all chugl graphics data movement
