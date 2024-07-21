#pragma once
#include <stack>
#include <iostream>
#include <string>
#include <array>

#include "x64Emitter.h"

class BFCompiler
{
public:
    inline const X64Emitter& getEmitter() const { return emitter; }

    inline void reset()
    {
        emitter.reset();
        std::memset(ram.data(), 0, sizeof(ram));
    }

    bool compile(const std::string& rom)
    {
        errorLocation currentLoc{};
        std::stack<errorLocation> openedBracketLocs;

        auto showErrorMsg = [](char bracket, errorLocation loc)
        {
            std::cout << "Compilation error! Unmatched '" << bracket << "' at " << std::to_string(loc.line) << ":" << std::to_string(loc.column);
        };

        std::stack<size_t> loopStack;

        emitter.write(0x55); // push rbp
        emitter.write(0x53); // push rbx

        // xor rbx, rbx
        emitter.write(0x48);
        emitter.write(0x31);
        emitter.write(0xDB);

        // mov rbp, ram.data()
        emitter.write(0x48);
        emitter.write(0xBD);
        emitter.writeQWord((uint64_t)ram.data());

        for (size_t i = 0; i < rom.size(); i++)
        {
            switch (rom[i])
            {
            case '>':
            {
                uint16_t adder = repeatingChars(rom.c_str(), i, '>');

                // add bx, adder
                emitter.write(0x66);
                emitter.write(0x81);
                emitter.write(0xC3);
                emitter.writeWord(adder);

                i += adder - 1;
                break;
            }
            case '<':
            {
                uint16_t subtract = repeatingChars(rom.c_str(), i, '<');

                // sub bx, subtract
                emitter.write(0x66);
                emitter.write(0x81);
                emitter.write(0xEB);
                emitter.writeWord(subtract);

                i += subtract - 1;
                break;
            }
            case '+':
            {
                uint8_t adder = static_cast<uint8_t>(repeatingChars(rom.c_str(), i, '+'));

                // add byte [rbp + rbx], adder
                emitPtrOpcode(0x04);
                emitter.write(adder);

                i += adder - 1;
                break;
            }
            case '-':
            {
                uint8_t subtract = static_cast<uint8_t>(repeatingChars(rom.c_str(), i, '-'));

                // sub byte [rbp + rbx], adder
                emitPtrOpcode(0x2C);
                emitter.write(subtract);

                i += subtract - 1;
                break;
            }
            case '.':
            {
                // mov cl, byte [rbp + rbx]
                emitter.write(0x8A);
                emitter.write(0x0C);
                emitter.write(0x2B);

                // call putchar
                emitFuncCall((size_t)putchar);

                break;
            }
            case ',':
            {
                // call getchar
                emitFuncCall((size_t)getchar);

                // mov byte[rbp + rbx], al
                emitter.write(0x88);
                emitter.write(0x04);
                emitter.write(0x2B);

                break;
            }
            case '[':
            {
                emitCmp0();

                // je endLoop
                emitter.write(0x0F);
                emitter.write(0x84);

                // loop end is not known yet, so address is pushed to the stack to be patched later
                loopStack.push(emitter.codeSize());
                emitter.writeDWord(0x00);

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

                size_t bracketPatchAddr = loopStack.top();
                loopStack.pop();

                emitCmp0();

                // jne beginLoop
                emitter.write(0x0F);
                emitter.write(0x85);
                emitter.writeDWord(bracketPatchAddr - emitter.codeSize());

                size_t currentAddr = emitter.codeSize();

                emitter.setCodePos(bracketPatchAddr);
                emitter.writeDWord(currentAddr - bracketPatchAddr - 4);
                emitter.setCodePos(currentAddr);

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

        emitter.write(0x5B); // pop rbx
        emitter.write(0x5D); // pop rbp
        emitter.write(0xC3); // ret

        return true;
    }

private:
    X64Emitter emitter;
    std::array<uint8_t, 1 << 16> ram{};

    inline void emitPtrOpcode(uint8_t opcode)
    {
        // add or sub [rbp + rbx]
        emitter.write(0x80);
        emitter.write(opcode);
        emitter.write(0x2B);
    }
    void emitFuncCall(size_t funcAddr)
    {
        // sub rsp, 32
        emitter.write(0x48);
        emitter.write(0x81);
        emitter.write(0xEC);
        emitter.writeDWord(32);

        // call funcAddr
        emitter.write(0x48);
        emitter.write(0xB8);
        emitter.writeQWord(funcAddr);
        emitter.write(0xFF);
        emitter.write(0xD0);

        // add rsp, 32
        emitter.write(0x48);
        emitter.write(0x81);
        emitter.write(0xC4);
        emitter.writeDWord(32);
    }
    inline void emitCmp0()
    {
        // cmp byte [rbx + rbp], 0
        emitter.write(0x80);
        emitter.write(0x3C);
        emitter.write(0x2B);
        emitter.write(0x00);
    }

    uint16_t repeatingChars(const char* str, uint32_t ind, char val) const
    {
        uint16_t num = 0;

        while (str[ind++] == val)
            num++;

        return num;
    }

    struct errorLocation
    {
        uint32_t line{ 1 };
        uint32_t column{ 1 };
    };
};