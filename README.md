# Encoding Stub

Buildable Raceboat `IEncodingComponent` plugin template. It contains a Base64 reference implementation extracted and adapted from `plugins/decomposed-exemplars/source/encoding/PluginCommsTwoSixBase64Encoding.*`, with SDK-mock tests, manifest/kit packaging, and a name-aware generator.

## Generate a plugin

Run from the RACEGEN root. The output directory must not already exist:

```bash
python3 plugins/encoding-stub/new_encoding.py \
  --name Base64 \
  --mime-type text/plain \
  --max-bytes 1000000 \
  --encoding-time-seconds 0.25 \
  --out /home/ubuntu/racegen/base64-encoding
```

`--name` is CamelCase and derives the encoding id, class, CMake target, library, and kit directory. The MIME type, capacity, and time are mandatory so a generated manifest cannot silently inherit sample values. Supply values established by the design and benchmark. This template's codec is Base64; for any other format, replace the encode/decode implementation and add format-specific validity tests before using the generated plugin.

## Build and test

The builder and runtime images must use the same tag. The default is `main`:

```bash
./build_artifacts_in_docker_image.sh --image-tag main
./run_unit_tests.sh --image-tag main
```

Artifacts are staged under `kit/artifacts/linux-<arch>-{client,server}/<PluginName>/`. Unit tests are built in the plugin-builder image and run in the runtime image. The test binary exercises byte-exact round trips, empty and all-byte inputs, malformed Base64, over-capacity inputs, declared properties, and SDK completion callbacks.

## Integration

The generated manifest registers only this encoding; it does not include a transport or user model. Add the generated plugin and a dedicated composition to the local integration harness, keeping `plugins/decomposed-exemplars` unchanged. The initial composition should use `twoSixIndirect` and this encoding; later test compatibility with a transport that declares the matching MIME type.