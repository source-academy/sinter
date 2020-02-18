# Sinter

Name etymology: <strong>S</strong>ource <strong>inter</strong>preter. (This isn't a direct Source interpreter though.)

This is an implementation of the Source language intended for microcontroller platforms like an Arduino.

In order to avoid having to do lexing and parsing on extremely low-power devices, we first compile Source to a VM designed to suit Source, and then interpret the VM bytecode. It is expected that the compiler will be integrated into the Source Academy; the compiler is thus written in TypeScript.

## Specifications

The `spec` folder contains documentation on the Sinter VM.

* [VM specification: `vm.md`](spec/vm.md)
