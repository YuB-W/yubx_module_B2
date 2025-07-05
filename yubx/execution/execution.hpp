#include "Windows.h"
#include "iostream"
#include "vector"
#include "VM/src/lobject.h"
#include "VM/src/lstate.h"
#include "VM/src/lapi.h"
#include "update.hpp"
#include "globals.hpp"
#include "Compiler/include/luacode.h"
#include "luau/BytecodeBuilder.h"
#include "luau/BytecodeUtils.h"
#include "luau/Compiler.h"
#include "zstd/include/zstd/zstd.h"
#include "zstd/include/zstd/xxhash.h"
#include "roblox/task_scheduler/task_scheduler.hpp"
#include "lualib.h"

struct lua_State;

class execution
{
public:
	static void execute_script(lua_State* l, const std::string& script);
};