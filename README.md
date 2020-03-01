# Sinter

Name etymology: <strong>S</strong>VM <strong>inter</strong>preter. (This isn't a direct Source interpreter though.)

This is an implementation of the Source language intended for microcontroller platforms like an Arduino.

In order to avoid having to do lexing and parsing on extremely low-power devices, we first compile Source to a VM designed to suit Source, and then interpret the VM bytecode.

## Specifications

We follow the [Source VM specification](https://github.com/source-academy/js-slang/wiki/SVML-Specification) as in the js-slang wiki.
