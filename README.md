# MegaBF

Fast Brainfuck AOT/JIT compiler for x86-64 windows, written in C++

## Build

Only windows is supported using MSVC or Clang in Visual Studio.

## Usage:

```
MegaBF -filepath
```
Or open without parameters to type the source code, then press CTRL + Z to run the program.

After running the program, file x64_output.txt is created, which contains raw x64 hex string which you can disassemble.

![mandelbrot](https://github.com/user-attachments/assets/576a54f9-4218-4a32-9247-a8f314a84459)

## License

This project is licensed under the MIT License - see the LICENSE.md file for details
