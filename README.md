# NanoGL compatibility layer

NanoGL exposes a legacy-style OpenGL API over an OpenGL ES backend. In mangled GCC/Clang builds, the wrappers export both the `pgl*` names and conventional `gl*` symbols, so older objects can link against the compatibility layer without needing one hand-written alias per entry point.

The compatibility layer is not a complete implementation of every desktop OpenGL 1.x behavior. Basic display lists record/replay immediate-mode begin/end, vertex, color, normal, texcoord, and nested-list commands; other state and extension commands are not captured. `glTexGen*` supports object-linear and eye-linear coordinates for immediate-mode vertices, while sphere/reflection/normal-map modes and client-array draws with texgen are rejected. ARB VBO map/unmap is emulated with a CPU shadow of buffer data; it cannot reflect changes made only by the GPU. `glGetTexImage` is emulated from a shadow copy for supported uncompressed client-memory uploads. Compressed-upload bytes are available through `glGetCompressedTexImageARB` when the original compressed upload is tracked. Images changed by GPU copies/FBOs, compressed subimages, or otherwise untracked operations return an error. `glGetTexLevelParameteriv` reports metadata for tracked texture levels. Depth-buffer `glReadPixels` remains unsupported. Polygon point/line rasterization (`glPolygonMode` other than `GL_FILL`) is rejected. Quad and quad-strip drawing is converted to GLES triangles for immediate mode and client-memory draw calls; VBO-backed quad conversion is rejected because the index data cannot safely be inspected. The renderer-specific GPU extension families used by NearChuckle (ARB/NV program objects, NV combiners, NV fences/occlusion queries, ATI separate-stencil modes, and related calls) are still not implemented; this compatibility layer therefore cannot run that renderer. Optional OES framebuffer procedures are guarded and report errors when unavailable.

Run the mocked regression suite:

```sh
gcc -DNANOGL_MANGLE_PREPEND=1 -I. -IGL \
  nanogl.c nanoWrap.c nanoCompat.c tests/test_batch.c \
  -ldl -o /tmp/nanogl-tests
/tmp/nanogl-tests
```

See [`demos/README.md`](demos/README.md) for building the headless demo and details about `demos/compat_result.svg`.
