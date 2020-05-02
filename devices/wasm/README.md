# Sinter WASM

This is a demo of Sinter on WASM.

## Building

You need [Emscripten](https://emscripten.org/), Node and Yarn installed.

```sh
mkdir build
cd build
emcmake cmake ../wasm
make -j$(nproc)
cd ../web
yarn install && yarn build
```
