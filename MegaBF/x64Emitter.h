#include <cstdint>
#include <array>
#include <Windows.h>

class X64Emitter
{
public:
	X64Emitter()
	{
		VirtualProtect(code.data(), sizeof(code), PAGE_EXECUTE_READWRITE, &oldProtect);
	}
	~X64Emitter()
	{
		VirtualProtect(code.data(), sizeof(code), oldProtect, &oldProtect);
	}

	inline void write(uint8_t byte)
	{
		code[codePos++] = byte;
	}

	inline void writeWord(uint16_t word)
	{
		*reinterpret_cast<uint16_t*>(&code[codePos]) = word;
		codePos += 2;
	}
	inline void writeDWord(uint32_t dword)
	{
		*reinterpret_cast<uint32_t*>(&code[codePos]) = dword;
		codePos += 4;
	}
	inline void writeQWord(uint64_t quad)
	{
		*reinterpret_cast<uint64_t*>(&code[codePos]) = quad;
		codePos += 8;
	}

	inline void execute() const
	{
		reinterpret_cast<void(*)()>(code.data())();
	}

	inline void setCodePos(size_t newPos) { codePos = newPos; }
	inline void reset() { codePos = 0; }

	size_t codeSize() const { return codePos; }
	const uint8_t* codePtr() const { return code.data(); }
private:
	// 1 MB code array
	std::array<uint8_t, 1 << 20> code;
	size_t codePos { 0 };

	DWORD oldProtect;
};