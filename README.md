# NanoGL compatibility layer

NanoGL exposes a legacy-style OpenGL API over an OpenGL ES backend. In mangled GCC/Clang builds, the wrappers export both the `pgl*` names and conventional `gl*` symbols, so older objects can link against the compatibility layer without needing one hand-written alias per entry point.

The compatibility layer is not a complete implementation of every desktop OpenGL 1.x behavior. GLES has no direct equivalents for several desktop commands and modes: display-list execution (`glCallList`), texture-coordinate generation (`glTexGen*`), polygon point/line rasterization (`glPolygonMode` other than `GL_FILL`), and depth-buffer `glReadPixels` are rejected with wrapper errors. Quad and quad-strip drawing is converted to GLES triangles for immediate mode and client-memory draw calls; VBO-backed quad conversion is rejected because the index data cannot safely be inspected. Optional OES framebuffer procedures are guarded and report errors when unavailable.

Run the mocked regression suite:

```sh
gcc -DNANOGL_MANGLE_PREPEND=1 -I. -IGL \
  nanogl.c nanoWrap.c nanoCompat.c tests/test_batch.c \
  -ldl -o /tmp/nanogl-tests
/tmp/nanogl-tests
```

See [`demos/README.md`](demos/README.md) for building the headless demo and details about `demos/compat_result.svg`.
