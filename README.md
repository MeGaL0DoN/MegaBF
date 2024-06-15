# MegaBF

Brainfuck AOT/JIT compiler to x64 written in C++

## Build

Currently only windows is supported, open MegaBF.sln in visual studio and build.

## Usage:

```
MegaBF -filepath
```
Or open without parameters to type the source code, then press CTRL + Z (on windows) or CTRL + D (on linux) to run the program.

After running the program, file x64_output.txt is created, which contains raw x64 hex string which you can disassemble.

## License

This project is licensed under the MIT License - see the LICENSE.md file for details
