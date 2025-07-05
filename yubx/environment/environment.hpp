#pragma once
#include "Windows.h"
#include "iostream"
#include "vector"
#include <string>

#include "VM/include/lua.h"
#include "VM/include/lualib.h"
#include "VM/src/lcommon.h"
#include "VM/src/lstring.h"
#include "VM/src/lfunc.h"
#include "VM/src/lmem.h"
#include "VM/src/lgc.h"
#include "VM/src/ltable.h"

#include "VM/src/lobject.h"
#include "VM/src/lstate.h"
#include "VM/src/lapi.h"
#include "VM/src/ldo.h"
#include "VM/src/lgc.h"

class environment
{
public:
	static void initialize(lua_State* l);
};