# NanoGL demos

`compat_result.svg` is a headless rendering of the legacy immediate-mode scene in `render_demo.c`. The demo routes `glBegin/glEnd`, quads, `glArrayElement` from client arrays, per-vertex normals, RGBA colors, and 4-component texture coordinates through NanoGL's GLES draw batching, then captures the GLES draw calls in a tiny SVG backend.

Build and render from the repository root:

```sh
gcc -DNANOGL_MANGLE_PREPEND=1 -I. -IGL \
  nanogl.c nanoWrap.c nanoCompat.c demos/render_demo.c \
  -ldl -o /tmp/nanogl_render_demo
/tmp/nanogl_render_demo demos/compat_result.svg
```

With Waf, configure with `--demos` and then build to include `nanogl_render_demo`.

The SVG is a deterministic software-rendered demo of NanoGL's converted draw stream; it does not require a platform GLES context. It is useful for verifying primitive conversion and the interleaved vertex attributes, but it is not a substitute for testing on a real GLES device.
