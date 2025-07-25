#include "environment.hpp"
#include "Http/Http.h"
#include <roblox/task_scheduler/task_scheduler.hpp>
#include <execution/execution.hpp>
#include <filesystem>
#include <fstream>
#include <thread>

void register_env_functions(lua_State* l, const luaL_Reg* functions) {
    lua_pushvalue(l, LUA_GLOBALSINDEX);
    luaL_register(l, nullptr, functions);
    lua_pop(l, 1);
}

void register_env_members(lua_State* l, const luaL_Reg* functions, const std::string& global_name) {
    luaL_register(l, global_name.c_str(), functions);
}

void register_to_global(lua_State* l, const luaL_Reg* functions, const std::string& global_name) {
    lua_getglobal(l, global_name.c_str());
    if (lua_istable(l, -1)) {
        lua_setreadonly(l, -1, false);
        luaL_register(l, nullptr, functions);
        lua_setreadonly(l, -1, true);
    }
    lua_pop(l, 1);
}
namespace YuBX {

    int identifyexecutor(lua_State* L) {
        luaL_trimstack(L, 0);
        lua_pushstring(L, "yubx");
        lua_pushstring(L, "2.0.0");
        return 2;
    };

    int getexecutorname(lua_State* L) {
        luaL_trimstack(L, 0);
        lua_pushstring(L, "yubx");
        return 1;
    };


    int loadstring(lua_State* L) {
        luaL_trimstack(L, 2);
        luaL_checktype(L, 1, LUA_TSTRING);
        auto Source = lua_tostring(L, 1);
        auto ChunkName = luaL_optstring(L, 2, "@yubx");

        auto Bytecode = execution::CompileScript(Source);

        if (luau_load(L, ChunkName, Bytecode.c_str(), Bytecode.length(), 0) != LUA_OK) {
            lua_pushnil(L);
            lua_pushvalue(L, -2);
            return 2;
        }

        Closure* Function = lua_toclosure(L, -1);
        if (Function && Function->l.p)
        {
            task_scheduler::set_proto_capabilities(((Closure*)lua_topointer(L, -1))->l.p, &max_caps);
        }

        lua_setsafeenv(L, LUA_GLOBALSINDEX, false);
        return 1;
    };

    namespace Metatable {
        int getrawmetatable(lua_State* L) {
            luaL_trimstack(L, 1);
            luaL_checkany(L, 1);
            if (!lua_getmetatable(L, 1))
                lua_pushnil(L);
            return 1;
        }

        int setrawmetatable(lua_State* L) {
            luaL_trimstack(L, 2);
            luaL_checkany(L, 1);
            luaL_checktype(L, 2, LUA_TTABLE);
            lua_setmetatable(L, 1);
            lua_pushvalue(L, 1);
            return 1;
        }

        int setreadonly(lua_State* L) {
            luaL_trimstack(L, 2);
            luaL_checktype(L, 1, LUA_TTABLE);
            luaL_checktype(L, 2, LUA_TBOOLEAN);
            hvalue(luaA_toobject(L, 1))->readonly = lua_toboolean(L, 2);
            return 0;
        }

        int isreadonly(lua_State* L) {
            luaL_trimstack(L, 1);
            luaL_checktype(L, 1, LUA_TTABLE);
            lua_pushboolean(L, hvalue(luaA_toobject(L, 1))->readonly);
            return 1;
        }

        int makewriteable(lua_State* L) {
            luaL_trimstack(L, 1);
            luaL_checktype(L, 1, LUA_TTABLE);
            hvalue(luaA_toobject(L, 1))->readonly = FALSE;
            return 0;
        }

        int makereadonly(lua_State* L) {
            luaL_trimstack(L, 1);
            luaL_checktype(L, 1, LUA_TTABLE);
            hvalue(luaA_toobject(L, 1))->readonly = TRUE;
            return 0;
        }

        int getnamecallmethod(lua_State* L) {
            luaL_trimstack(L, 0);
            if (!L->namecall)
                lua_pushnil(L);
            else {
                setsvalue(L, L->top, L->namecall);
                L->top++;
            }
            return 1;
        }

        int setnamecallmethod(lua_State* L) {
            luaL_trimstack(L, 1);
            luaL_checktype(L, 1, LUA_TSTRING);
            if (!L->namecall)
                return 0;
            L->namecall = tsvalue(luaA_toobject(L, 1));
            return 0;
        }

        int gettenv(lua_State* L) {
            luaL_trimstack(L, 1);
            luaL_checktype(L, 1, LUA_TTHREAD);
            lua_State* ls = (lua_State*)lua_topointer(L, 1);
            LuaTable* tab = hvalue(luaA_toobject(ls, LUA_GLOBALSINDEX));

            sethvalue(L, L->top, tab);
            L->top++;

            return 1;
        };

        int getgenv(lua_State* L) {
            luaL_trimstack(L, 0);
            lua_pushvalue(L, LUA_ENVIRONINDEX);
            return 1;
        }

        int getrenv(lua_State* L) {
            luaL_trimstack(L, 0);
            lua_State* RobloxState = globals::yubx_state;
            LuaTable* clone = luaH_clone(L, RobloxState->gt);

            lua_rawcheckstack(L, 1);
            luaC_threadbarrier(L);
            luaC_threadbarrier(RobloxState);

            L->top->value.p = clone;
            L->top->tt = LUA_TTABLE;
            L->top++;

            lua_rawgeti(L, LUA_REGISTRYINDEX, 2);
            lua_setfield(L, -2, "_G");
            lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
            lua_setfield(L, -2, "shared");
            return 1;
        }

        int getgc(lua_State* L) {
            luaL_trimstack(L, 1);
            const bool includeTables = luaL_optboolean(L, 1, false);

            lua_newtable(L);
            lua_newtable(L);

            lua_pushstring(L, "kvs");
            lua_setfield(L, -2, "__mode");

            lua_setmetatable(L, -2);

            typedef struct {
                lua_State* luaThread;
                bool includeTables;
                int itemsFound;
            } GCOContext;

            auto GCContext = GCOContext{ L, includeTables, 0 };

            const auto oldGCThreshold = L->global->GCthreshold;
            L->global->GCthreshold = SIZE_MAX;

            luaM_visitgco(L, &GCContext, [](void* ctx, lua_Page* page, GCObject* gcObj) -> bool {
                const auto context = static_cast<GCOContext*>(ctx);
                const auto luaThread = context->luaThread;

                if (isdead(luaThread->global, gcObj))
                    return false;

                const auto gcObjectType = gcObj->gch.tt;
                if (gcObjectType == LUA_TFUNCTION || gcObjectType == LUA_TTHREAD || gcObjectType == LUA_TUSERDATA ||
                    gcObjectType == LUA_TLIGHTUSERDATA ||
                    gcObjectType == LUA_TBUFFER || gcObjectType == LUA_TTABLE && context->includeTables) {
                    luaThread->top->value.gc = gcObj;
                    luaThread->top->tt = gcObjectType;
                    incr_top(luaThread);

                    const auto newTableIndex = context->itemsFound++;
                    lua_rawseti(luaThread, -2, newTableIndex);
                }

                return false;
                });

            L->global->GCthreshold = oldGCThreshold;

            return 1;
        };
    }


    namespace Filesystem {
        static std::filesystem::path a = getenv("LOCALAPPDATA");
        static std::filesystem::path exploit_name = a / "YuBX";
        static std::filesystem::path c = exploit_name / "workspace";
        static std::filesystem::path autoe = exploit_name / "Autoexec";


        std::string getWorkspaceFolder() {
            if (!std::filesystem::exists(c)) {
                std::filesystem::create_directories(c);
            }
            return c.string() + "\\";
        };

        std::string getautoexecFolder() {
            if (!std::filesystem::exists(autoe)) {
                std::filesystem::create_directories(autoe);
            }
            return autoe.string() + "\\";
        }

        static void autoExecuteFolderScripts() {
            std::filesystem::path folder = getautoexecFolder();

            for (const auto& entry : std::filesystem::directory_iterator(folder)) {
                if (!entry.is_regular_file()) continue;

                std::string ext = entry.path().extension().string();
                if (ext == ".luau" || ext == ".txt" || ext == ".lua" || ext == ".yubx") {
                    std::ifstream file(entry.path());
                    if (!file.is_open()) continue;

                    std::string script((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
                    execution::execute_script(globals::yubx_state, "if not game:IsLoaded() then game.Loaded:Wait() end wait(1) \n" + script);
                }
            }
        }

        void _SplitString(std::string Str, std::string By, std::vector<std::string>& Tokens)
        {
            Tokens.push_back(Str);
            const auto splitLen = By.size();
            while (true)
            {
                auto frag = Tokens.back();
                const auto splitAt = frag.find(By);
                if (splitAt == std::string::npos)
                    break;
                Tokens.back() = frag.substr(0, splitAt);
                Tokens.push_back(frag.substr(splitAt + splitLen, frag.size() - (splitAt + splitLen)));
            }
        }

        int makefolder(lua_State* L) {
            luaL_trimstack(L, 1);
            luaL_checktype(L, 1, LUA_TSTRING);

            std::string Path = luaL_checklstring(L, 1, 0);

            std::replace(Path.begin(), Path.end(), '\\', '/');
            std::vector<std::string> Tokens;
            _SplitString(Path, "/", Tokens);

            std::string CurrentPath = getWorkspaceFolder();
            std::replace(CurrentPath.begin(), CurrentPath.end(), '\\', '/');

            for (const auto& Token : Tokens) {
                CurrentPath += Token + "/";

                if (!std::filesystem::is_directory(CurrentPath))
                    std::filesystem::create_directory(CurrentPath);
            }

            return 0;
        };

        int isfile(lua_State* L) {
            luaL_trimstack(L, 1);
            luaL_checktype(L, 1, LUA_TSTRING);

            std::string Path = luaL_checklstring(L, 1, 0);

            std::string FullPath = getWorkspaceFolder() + Path;
            std::replace(FullPath.begin(), FullPath.end(), '\\', '/');

            lua_pushboolean(L, std::filesystem::is_regular_file(FullPath));
            return 1;
        };

        int readfile(lua_State* L) {
            luaL_trimstack(L, 1);
            luaL_checktype(L, 1, LUA_TSTRING);

            std::string Path = luaL_checklstring(L, 1, 0);

            std::string FullPath = getWorkspaceFolder() + Path;
            std::replace(FullPath.begin(), FullPath.end(), '\\', '/');

            if (std::to_string(std::filesystem::is_regular_file(FullPath)) == "1")
            {
                std::ifstream File(FullPath, std::ios::binary);
                if (!File)
                    luaL_error(L, "Failed to open file: %s", FullPath.c_str());

                std::string Content((std::istreambuf_iterator<char>(File)), std::istreambuf_iterator<char>());
                File.close();

                lua_pushlstring(L, Content.data(), Content.size());
                return 1;
            }
            else
                luaL_error(L, "Failed to open file: %s", FullPath.c_str());

            return 0;
        }


        int writefile(lua_State* L) {
            luaL_trimstack(L, 2);
            luaL_checktype(L, 1, LUA_TSTRING);
            luaL_checktype(L, 2, LUA_TSTRING);

            size_t contentSize = 0;
            std::string Path = luaL_checklstring(L, 1, 0);
            const auto Content = luaL_checklstring(L, 2, &contentSize);

            std::replace(Path.begin(), Path.end(), '\\', '/');

            std::vector<std::string> DisallowedExtensions =
            {
                ".exe", ".scr", ".bat", ".com", ".csh", ".msi", ".vb", ".vbs", ".vbe", ".ws", ".wsf", ".wsh", ".ps1"
            };

            for (std::string Extension : DisallowedExtensions) {
                if (Path.find(Extension) != std::string::npos) {
                    luaL_error(L, ("forbidden file extension"));
                }
            }

            std::string FullPath = getWorkspaceFolder() + Path;
            std::replace(FullPath.begin(), FullPath.end(), '\\', '/');
            std::ofstream fileToWrite(FullPath, std::ios::beg | std::ios::binary);
            fileToWrite.write(Content, contentSize);
            fileToWrite.close();

            return 0;
        };

        int listfiles(lua_State* L) {
            luaL_trimstack(L, 1);
            luaL_checktype(L, 1, LUA_TSTRING);

            std::string Path = luaL_checklstring(L, 1, 0);

            std::string FullPath = getWorkspaceFolder() + Path;
            std::replace(FullPath.begin(), FullPath.end(), '\\', '/');
            std::string halfPath = getWorkspaceFolder();
            std::string workspace = ("\\Workspace\\");

            if (!std::filesystem::is_directory(FullPath))
                luaL_error(L, ("folder does not exist"));

            lua_createtable(L, 0, 0);
            int i = 1;
            for (const auto& entry : std::filesystem::directory_iterator(FullPath)) {
                std::string path = entry.path().string().substr(halfPath.length());

                lua_pushinteger(L, i);
                lua_pushstring(L, path.c_str());
                lua_settable(L, -3);
                i++;
            }

            return 1;
        };

        int isfolder(lua_State* L) {
            luaL_trimstack(L, 1);
            luaL_checktype(L, 1, LUA_TSTRING);

            std::string Path = luaL_checklstring(L, 1, 0);

            std::string FullPath = getWorkspaceFolder() + Path;
            std::replace(FullPath.begin(), FullPath.end(), '\\', '/');

            lua_pushboolean(L, std::filesystem::is_directory(FullPath));

            return 1;
        };

        int delfolder(lua_State* L) {
            luaL_trimstack(L, 1);
            luaL_checktype(L, 1, LUA_TSTRING);

            std::string Path = luaL_checklstring(L, 1, 0);

            std::string FullPath = getWorkspaceFolder() + Path;
            std::replace(FullPath.begin(), FullPath.end(), '\\', '/');

            if (!std::filesystem::remove_all(FullPath))
                luaL_error(L, ("folder does not exist"));

            return 0;
        };

        int delfile(lua_State* L) {
            luaL_trimstack(L, 1);
            luaL_checktype(L, 1, LUA_TSTRING);

            std::string Path = luaL_checklstring(L, 1, 0);

            std::string FullPath = getWorkspaceFolder() + Path;
            std::replace(FullPath.begin(), FullPath.end(), '\\', '/');

            if (!std::filesystem::remove(FullPath))
                luaL_error(L, ("file does not exist"));

            return 0;
        };

        int loadfile(lua_State* L) {
            luaL_trimstack(L, 1);
            luaL_checktype(L, 1, LUA_TSTRING);

            std::string Path = luaL_checklstring(L, 1, 0);

            std::string FullPath = getWorkspaceFolder() + Path;
            std::replace(FullPath.begin(), FullPath.end(), '\\', '/');

            if (!std::filesystem::is_regular_file(FullPath))
                luaL_error(L, ("file does not exist"));

            std::ifstream File(FullPath);
            std::string Content((std::istreambuf_iterator<char>(File)), std::istreambuf_iterator<char>());
            File.close();

            lua_pop(L, lua_gettop(L));

            lua_pushlstring(L, Content.data(), Content.size());

            return loadstring(L);
        };

        int appendfile(lua_State* L) {
            luaL_trimstack(L, 2);
            luaL_checktype(L, 1, LUA_TSTRING);
            luaL_checktype(L, 2, LUA_TSTRING);

            size_t contentSize = 0;
            std::string Path = luaL_checklstring(L, 1, 0);
            const auto Content = luaL_checklstring(L, 2, &contentSize);

            std::replace(Path.begin(), Path.end(), '\\', '/');

            std::string FullPath = getWorkspaceFolder() + Path;
            std::replace(FullPath.begin(), FullPath.end(), '\\', '/');

            std::ofstream fileToWrite(FullPath, std::ios::binary | std::ios::app);
            fileToWrite << Content;
            fileToWrite.close();

            return 0;
        };

        int getcustomasset(lua_State* L) {
            luaL_trimstack(L, 1);
            luaL_checktype(L, 1, LUA_TSTRING);

            std::string assetPath = lua_tostring(L, 1);
            std::string fullPathStr = getWorkspaceFolder() + assetPath;
            std::replace(fullPathStr.begin(), fullPathStr.end(), '\\', '/');
            std::filesystem::path FullPath = fullPathStr;

            if (!std::filesystem::is_regular_file(FullPath)) {
                luaL_error(L, "Failed to find local asset!");
                return 0;
            }

            std::filesystem::path customAssetsDir = std::filesystem::current_path()
                / "ExtraContent"
                / "YuBX";

            std::filesystem::path customAssetsFile = customAssetsDir / FullPath.filename();

            try {
                if (!std::filesystem::exists(customAssetsDir))
                    std::filesystem::create_directories(customAssetsDir);

                std::filesystem::copy_file(FullPath, customAssetsFile, std::filesystem::copy_options::update_existing);
            }
            catch (const std::exception& e) {
                luaL_error(L, std::format("Failed to copy asset: {}", e.what()).c_str());

                return 0;
            }

            std::string Final = "rbxasset://YuBX/" + customAssetsFile.filename().string();
            lua_pushlstring(L, Final.c_str(), Final.size());
            return 1;
        }
    }
    namespace HelpFuncs {
        using YieldReturn = std::function<int(lua_State* L)>;

        __forceinline bool CheckMemory(uintptr_t address) {
            if (address < 0x10000 || address > 0x7FFFFFFFFFFF) {
                return false;
            }

            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQuery(reinterpret_cast<void*>(address), &mbi, sizeof(mbi)) == 0) {
                return false;
            }

            if (mbi.Protect & PAGE_NOACCESS || mbi.State != MEM_COMMIT) {
                return false;
            }

            return true;
        }

        static void ThreadFunc(const std::function<YieldReturn()>& YieldedFunction, lua_State* L)
        {
            YieldReturn ret_func;

            try
            {
                ret_func = YieldedFunction();
            }
            catch (std::exception ex)
            {
                lua_pushstring(L, ex.what());
                lua_error(L);
            }

            lua_State* l_new = lua_newthread(L);

            const auto returns = ret_func(L);

            lua_getglobal(l_new, ("task"));
            lua_getfield(l_new, -1, ("defer"));

            lua_pushthread(L);
            lua_xmove(L, l_new, 1);

            for (int i = returns; i >= 1; i--)
            {
                lua_pushvalue(L, -i);
                lua_xmove(L, l_new, 1);
            }

            lua_pcall(l_new, returns + 1, 0, 0);
            lua_settop(l_new, 0);
        }

        static int YieldExecution(lua_State* L, const std::function<YieldReturn()>& YieldedFunction)
        {
            lua_pushthread(L);
            lua_ref(L, -1);
            lua_pop(L, 1);

            std::thread(ThreadFunc, YieldedFunction, L).detach();

            L->base = L->top;
            L->status = LUA_YIELD;

            L->ci->flags |= 1;
            return -1;
        }


        static void IsInstance(lua_State* L, int idx) {
            std::string typeoff = luaL_typename(L, idx);
            if (typeoff != "Instance")
                luaL_typeerrorL(L, 1, "Instance");
        };

        static bool IsClassName(lua_State* L, int idx, std::string className) {
            int originalArgCount = lua_gettop(L);

            if (lua_isnil(L, idx)) {
                return false;
            }

            lua_getglobal(L, "typeof");
            lua_pushvalue(L, idx);
            lua_pcall(L, 1, 1, 0);

            std::string resultType = luaL_checklstring(L, -1, nullptr);
            lua_pop(L, lua_gettop(L) - originalArgCount);

            if (resultType != "Instance") {
                return false;
            }

            lua_getfield(L, idx, "ClassName");
            std::string object_ClassName = luaL_checklstring(L, -1, nullptr);
            lua_pop(L, lua_gettop(L) - originalArgCount);

            lua_getfield(L, idx, "IsA");
            lua_pushvalue(L, idx);
            lua_pushlstring(L, className.data(), className.size());
            lua_pcall(L, 2, 1, 0);

            bool isAResult = lua_isboolean(L, -1) ? lua_toboolean(L, -1) : false;
            lua_pop(L, lua_gettop(L) - originalArgCount);

            if (!isAResult & object_ClassName != className)
                return false;

            return true;
        };

        static std::string GetPlaceId(lua_State* L) {
            lua_getglobal(L, "game");
            if (!lua_istable(L, -1)) return "";

            lua_getfield(L, -1, "PlaceId");
            std::string result;

            if (lua_isstring(L, -1))
                result = lua_tostring(L, -1);

            lua_pop(L, 2);
            return result;
        }

        static std::string GetGameId(lua_State* L) {
            lua_getglobal(L, "game");
            if (!lua_istable(L, -1)) return "";

            lua_getfield(L, -1, "GameId");
            std::string result;

            if (lua_isstring(L, -1))
                result = lua_tostring(L, -1);

            lua_pop(L, 2);
            return result;
        }

        static void SetNewIdentity(lua_State* L, int Identity) {
            L->userdata->Identity = Identity;

            std::int64_t Ignore[128];
            roblox::Impersonator(Ignore, &Identity, *(__int64*)((uintptr_t)L->userdata + 0x48));
        }

    }


    namespace Cache {
        int invalidate(lua_State* LS) {
            luaL_checktype(LS, 1, LUA_TUSERDATA);
            HelpFuncs::IsInstance(LS, 1);

            const auto Instance = *static_cast<void**>(lua_touserdata(LS, 1));
            
            lua_pushlightuserdata(LS, (void*)roblox::PushInstance);
            lua_gettable(LS, LUA_REGISTRYINDEX);

            lua_pushlightuserdata(LS, reinterpret_cast<void*>(Instance));
            lua_pushnil(LS);
            lua_settable(LS, -3);

            return 0;
        };

        int replace(lua_State* LS) {
            luaL_checktype(LS, 1, LUA_TUSERDATA);
            luaL_checktype(LS, 2, LUA_TUSERDATA);

            HelpFuncs::IsInstance(LS, 1);
            HelpFuncs::IsInstance(LS, 2);

            const auto Instance = *reinterpret_cast<uintptr_t*>(lua_touserdata(LS, 1));

            lua_pushlightuserdata(LS, (void*)roblox::PushInstance);
            lua_gettable(LS, LUA_REGISTRYINDEX);

            lua_pushlightuserdata(LS, (void*)Instance);
            lua_pushvalue(LS, 2);
            lua_settable(LS, -3);
            return 0;
        };

        int iscached(lua_State* LS) {
            luaL_checktype(LS, 1, LUA_TUSERDATA);
            HelpFuncs::IsInstance(LS, 1);
            const auto Instance = *static_cast<void**>(lua_touserdata(LS, 1));

            lua_pushlightuserdata(LS, (void*)roblox::PushInstance);
            lua_gettable(LS, LUA_REGISTRYINDEX);

            lua_pushlightuserdata(LS, Instance);
            lua_gettable(LS, -2);

            lua_pushboolean(LS, !lua_isnil(LS, -1));
            return 1;
        };

        int cloneref(lua_State* LS) {
            luaL_checktype(LS, 1, LUA_TUSERDATA);
            HelpFuncs::IsInstance(LS, 1);
            const auto OldUserdata = lua_touserdata(LS, 1);
            const auto NewUserdata = *reinterpret_cast<uintptr_t*>(OldUserdata);

            lua_pushlightuserdata(LS, (void*)roblox::PushInstance);

            lua_rawget(LS, -10000);
            lua_pushlightuserdata(LS, reinterpret_cast<void*>(NewUserdata));
            lua_rawget(LS, -2);

            lua_pushlightuserdata(LS, reinterpret_cast<void*>(NewUserdata));
            lua_pushnil(LS);
            lua_rawset(LS, -4);

            roblox::PushInstance(LS, (uintptr_t)OldUserdata);

            lua_pushlightuserdata(LS, reinterpret_cast<void*>(NewUserdata));
            lua_pushvalue(LS, -3);
            lua_rawset(LS, -5);

            return 1;
        };

        int compareinstances(lua_State* LS) {
            luaL_checktype(LS, 1, LUA_TUSERDATA);
            luaL_checktype(LS, 2, LUA_TUSERDATA);

            HelpFuncs::IsInstance(LS, 1);
            HelpFuncs::IsInstance(LS, 2);

            uintptr_t First = *reinterpret_cast<uintptr_t*>(lua_touserdata(LS, 1));
            if (!First)
                luaL_argerrorL(LS, 1, "Invalid instance");

            uintptr_t Second = *reinterpret_cast<uintptr_t*>(lua_touserdata(LS, 2));
            if (!Second)
                luaL_argerrorL(LS, 2, "Invalid instance");

            if (First == Second)
                lua_pushboolean(LS, true);
            else
                lua_pushboolean(LS, false);

            return 1;
        };
    };

}

std::string namecallhookscript = R"(
printidentity()
print(identifyexecutor())
)";

void environment::initialize(lua_State* l) {
    static const luaL_Reg yubx_misc[] = {
         { "getcustomasset", YuBX::Filesystem::getcustomasset },
         { "writefile",      YuBX::Filesystem::writefile },
         { "readfile",       YuBX::Filesystem::readfile },
         { "makefolder",     YuBX::Filesystem::makefolder },
         { "isfolder",       YuBX::Filesystem::isfolder },
         { "delfile",        YuBX::Filesystem::delfile },
         { "appendfile",     YuBX::Filesystem::appendfile },
         { "delfolder",      YuBX::Filesystem::delfolder },
         { "isfile",         YuBX::Filesystem::isfile },
         { "listfiles",      YuBX::Filesystem::listfiles },
         { "loadfile",       YuBX::Filesystem::loadfile },

        // { "compareinstances",  YuBX::Cache::compareinstances },
        // { "getthreadidentity", YuBX::getthreadidentity },
        // { "setthreadidentity", YuBX::setthreadidentity },
        // { "gethui",            YuBX::gethui },
        // { "cloneref",          YuBX::Cache::cloneref },
         { "getrawmetatable",   YuBX::Metatable::getrawmetatable },
         { "setrawmetatable",   YuBX::Metatable::setrawmetatable },
         { "isreadonly",        YuBX::Metatable::isreadonly },
         { "setreadonly",       YuBX::Metatable::setreadonly },
         { "getnamecallmethod", YuBX::Metatable::getnamecallmethod },
         { "setnamecallmethod", YuBX::Metatable::setnamecallmethod },
         { "make_writeable",    YuBX::Metatable::makewriteable },
         { "make_readonly",     YuBX::Metatable::makereadonly },
         { "makewriteable",     YuBX::Metatable::makewriteable },
         { "makereadonly",      YuBX::Metatable::makereadonly },
         { "identifyexecutor",  YuBX::identifyexecutor },
         { "getexecutorname",   YuBX::getexecutorname },
         { "loadstring",        YuBX::loadstring },
         { "getgenv",           YuBX::Metatable::getgenv },
         { "gettenv",           YuBX::Metatable::gettenv },
         { "getrenv",           YuBX::Metatable::getrenv },
         { "getgc",             YuBX::Metatable::getgc },
         { nullptr, nullptr }
    };


    register_env_functions(l, yubx_misc);

    Http::Register(l);  

    lua_newtable(l);
    lua_setglobal(l, "_G");

    lua_newtable(l);
    lua_setglobal(l, "shared");

    static const luaL_Reg cache_lib[] = {
        { "invalidate",        YuBX::Cache::invalidate },
        { "replace",           YuBX::Cache::replace },
        { "iscached",          YuBX::Cache::iscached },
        { "cloneref",          YuBX::Cache::cloneref },
        { "compareinstances",  YuBX::Cache::compareinstances },
        { nullptr, nullptr }
    };

    lua_newtable(l);
    luaL_register(l, nullptr, cache_lib);
    lua_setfield(l, LUA_GLOBALSINDEX, "cache");

    execution::execute_script(globals::yubx_state, namecallhookscript);
    YuBX::Filesystem::autoExecuteFolderScripts();
}
