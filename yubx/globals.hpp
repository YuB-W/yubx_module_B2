#pragma once
#include <cstdint>
#include <lua.h>
#include <Windows.h>
#include <queue>
#include <string>

#include "Compiler/include/luacode.h"
#include "luau/BytecodeBuilder.h"
#include "luau/BytecodeUtils.h"
#include "luau/Compiler.h"

inline uintptr_t max_caps = 0xEFFFFFFFFFFFFFFF;

class bytecode_encoder_t : public Luau::BytecodeEncoder {
	inline void encode(uint32_t* data, size_t count) override {
		for (auto i = 0; i < count;) {
			uint8_t op = LUAU_INSN_OP(data[i]);
			const auto opLength = Luau::getOpLength(static_cast<LuauOpcode>(op));
			const auto lookupTable = reinterpret_cast<BYTE*>(update::roblox::OpcodeLookupTable);
			uint8_t newOp = op * 227;
			newOp = lookupTable[newOp];
			data[i] = (newOp) | (data[i] & ~0xff);
			i += opLength;
		}
	}
};

namespace globals {
	inline lua_State* global_state;
	inline lua_State* yubx_state;

	inline std::queue<std::string> execution_queue = {};
}