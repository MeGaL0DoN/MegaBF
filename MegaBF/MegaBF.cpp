#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <array>
#include <stack>

#include <xbyak/xbyak.h>

#ifdef _WIN32
#include "shellapi.h"
#endif

static std::array<uint8_t, 1 << 16> ram{};
static std::string rom;

bool loadROM(const std::filesystem::path& path)
{
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs) return false;

    rom.resize(ifs.tellg());

    ifs.seekg(0, std::ios::beg);
    ifs.read(rom.data(), rom.size());

    return true;
}

uint32_t repeatingChars(uint32_t ind, char val)
{
    uint32_t num = 0;

    while (rom[ind++] == val)
        num++;

    return num;
}

struct BrainCode : Xbyak::CodeGenerator
{
public:
    BrainCode() : Xbyak::CodeGenerator(Xbyak::DEFAULT_MAX_CODE_SIZE, Xbyak::AutoGrow)
    { }

    // r12 is pointer to the RAM array.
    // r13 is brainfuck data pointer.
    
#define pointedData byte[r12 + r13]

    bool compile()
    {
        errorLocation currentLoc{};
        std::stack<errorLocation> openedBracketLocs;

        auto showErrorMsg = [](char bracket, errorLocation loc) 
        {
             std::cout << "Compilation error! Unmatched '" << bracket << "' at " << std::to_string(loc.line) << ":" << std::to_string(loc.column);
        };

        std::stack<Xbyak::Label> loopStack;

        push(rsp);
        mov(r12, (size_t)ram.data());
        xor_(r13, r13);

        for (size_t i = 0; i < rom.size(); i++)
        {
            switch (rom[i])
            {
            case '>':
            {
                uint32_t adder = repeatingChars(i, '>');
                add(r13w, adder);
                i += adder - 1;
                break;
            }
            case '<':
            {
                uint32_t subtract = repeatingChars(i, '<');
                sub(r13w, subtract);
                i += subtract - 1;
                break;
            }
            case '+':
            {
                uint32_t adder = repeatingChars(i, '+');
                add(pointedData, adder);
                i += adder - 1;
                break;
            }
            case '-':
            {
                uint32_t subtract = repeatingChars(i, '-');
                sub(pointedData, subtract);
                i += subtract - 1;
                break;
            }
            case '.':
            {
                mov(rcx, pointedData);
                sub(rsp, 32);

                mov(rax, (size_t)putchar);
                call(rax);
                add(rsp, 32);

                break;
            }
            case ',':
            {
                sub(rsp, 32);

                mov(rax, (size_t)getchar);
                call(rax);

                mov(pointedData, al);
                add(rsp, 32);

                break;
            }
            case '[':
            {
                Xbyak::Label beginLoop = L();
                Xbyak::Label endLoop;

                cmp(pointedData, 0);
                je(endLoop, Xbyak::CodeGenerator::T_NEAR);

                loopStack.push(beginLoop);
                loopStack.push(endLoop);

                openedBracketLocs.push(currentLoc);

                break;
            }
            case ']':
            {
                if (loopStack.empty())
                {
                    showErrorMsg(']', currentLoc);
                    return false;
                }

                Xbyak::Label endLoop = loopStack.top();
                loopStack.pop();

                Xbyak::Label beginLoop = loopStack.top();
                loopStack.pop();

                cmp(pointedData, 0);
                jne(beginLoop, Xbyak::CodeGenerator::T_NEAR);

                L(endLoop);

                openedBracketLocs.pop();
                break;
            }
            case '\n':
                currentLoc.line++;
                currentLoc.column = 0;
            }

            currentLoc.column++;
        }

        if (!loopStack.empty())
        {
            showErrorMsg('[', openedBracketLocs.top());
            return false;
        }

        pop(rsp);
        ret();

        return true;
    }
private:
    struct errorLocation
    {
        uint32_t line { 1 };
        uint32_t column { 1 };
    };
};

inline std::string hexStr(const uint8_t* data, int len)
{
    std::stringstream ss;
    ss << std::hex;

    for (int i(0); i < len; ++i)
        ss << std::setw(2) << std::setfill('0') << (int)data[i];

    return ss.str();
}

inline void dumpCode(const BrainCode& code)
{
    std::ofstream ofs("x64_output.txt");
    ofs << hexStr(code.getCode(), code.getSize());

    rom.clear();
    rom.shrink_to_fit();
}

void compileAndRun()
{
    BrainCode code;
    bool error{ false };

    try
    {
        error = !code.compile();
    }
    catch (...)
    {
        std::cout << "Compilation error! Source is invalid.";
        error = true;
    }

    if (error) return;

    code.ready();
    dumpCode(code);

    auto func = code.getCode<void(*)()>();
    auto start = std::chrono::high_resolution_clock::now();

    func();

    auto end = std::chrono::high_resolution_clock::now(); 
    double time = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "\n----------------\nProgram Executed. Time: " << time << " ms";
}

void clearConsole()
{
#if defined _WIN32
    system("cls");
#elif defined (__LINUX__) || defined(__gnu_linux__) || defined(__linux__)
    std::cout << u8"\033[2J\033[1;1H";
#elif defined (__APPLE__)
    system("clear");
#endif
}

inline void reset()
{
    getchar();
    clearConsole();
    std::cin.clear();

    rom = "";
    std::memset(ram.data(), 0, sizeof(ram));
}

int main(int argc, char* argv[])
{
    if (argc > 1)
    {
#ifdef _WIN32
        auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
#endif
        if (!loadROM(argv[1]))
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
                rom += line +"\n";

            if (rom[0] == '\n')
                rom.erase(rom.begin());

            clearConsole();
            compileAndRun();
            reset();
        }
    }

    return 0;
}