#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

#include "BFCompiler.h"

std::string rom;
BFCompiler bfCompiler;

bool loadROM(const std::filesystem::path& path)
{
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs) return false;

    rom.resize(ifs.tellg());

    ifs.seekg(0, std::ios::beg);
    ifs.read(rom.data(), rom.size());

    return true;
}

inline std::string hexStr(const uint8_t* data, int len)
{
    std::stringstream ss;
    ss << std::hex;

    for (int i(0); i < len; ++i)
        ss << std::setw(2) << std::setfill('0') << (int)data[i];

    return ss.str();
}

inline void dumpCode()
{
    std::ofstream ofs("x64_output.txt");
    ofs << hexStr(bfCompiler.getEmitter().codePtr(), bfCompiler.getEmitter().codeSize());
}

void compileAndRun()
{
    bool error{ false };

    try
    {
        error = !bfCompiler.compile(rom);
    }
    catch (...)
    {
        std::cout << "Compilation error! Source is invalid.";
        error = true;
    }

    if (error) return;

    dumpCode();

    auto start = std::chrono::high_resolution_clock::now();

    bfCompiler.getEmitter().execute();

    auto end = std::chrono::high_resolution_clock::now(); 
    double time = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "\n----------------\nProgram Executed. Time: " << time << " ms";
}

void clearConsole()
{
    system("cls");
}

inline void reset()
{
    getchar();
    clearConsole();
    std::cin.clear();

    rom = "";
    bfCompiler.reset();
}

int main(int argc, char* argv[])
{
    if (argc > 1)
    {
        auto argW = CommandLineToArgvW(GetCommandLineW(), &argc);

        if (!loadROM(argW[1]))
            std::cout << "Error loading the file!";
        else
            compileAndRun();
    }
    else
    {
        while (true)
        {
            std::cout << "Enter the code: ";

            std::string line;

            while (rom == "" || std::getline(std::cin, line))
                rom += line + "\n";

            if (rom[0] == '\n')
                rom.erase(rom.begin());

            clearConsole();
            compileAndRun();
            reset();
        }
    }

    return 0;
}