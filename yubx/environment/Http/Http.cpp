#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>    
#include <ws2tcpip.h>    
#include <Windows.h>     

#include <curl/curl.h>   
#include <lualib.h>
#include "Http.h"
#include <unordered_map>

#include <roblox/task_scheduler/task_scheduler.hpp>
#include <execution/execution.hpp>
#include <filesystem>
#include <fstream>
#include <thread>

#include "cpr/cpr.h"
#include "cpr/cprtypes.h"
#include "HttpStatus/HttpStatus.hpp" 
#include "nlohmann/json.hpp"

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output)
{
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}
#define oxorany(x) x 
#define RegisterFunction(L, Func, Name) lua_pushcclosure(L, Func, Name, 0); \

#define RegisterMember(L, Func, Name) lua_pushcclosure(L, Func, Name, 0); \
lua_setfield(L, -2, Name);

std::unordered_map<std::string, std::string> Caching;

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

using json = nlohmann::json;

int HttpGet(lua_State* L) {
    luaL_trimstack(L, 2);
    luaL_checktype(L, 2, LUA_TSTRING);

    const char* Url = luaL_checklstring(L, 2, nullptr);
    bool CacheOrNot = lua_isboolean(L, 3) ? lua_toboolean(L, 3) : false;

    std::string UrlStr(Url);
    if (UrlStr.find("http://") != 0 && UrlStr.find("https://") != 0) {
        luaL_argerror(L, 2, "Invalid protocol (expected 'http://' or 'https://')");
        return 0;
    }

    if (CacheOrNot) {
        const auto it = Caching.find(UrlStr);
        if (it != Caching.end()) {
            lua_pushlstring(L, it->second.data(), it->second.size());
            return 1;
        }
    }

    std::string GameId = HelpFuncs::GetGameId(L);
    std::string PlaceId = HelpFuncs::GetPlaceId(L);

    using Json = nlohmann::json;
    Json sessionIdJson;
    sessionIdJson["GameId"] = GameId;
    sessionIdJson["PlaceId"] = PlaceId;

    cpr::Header headers;

    headers.insert({ "User-Agent", "Roblox/WinInet" });
    headers.insert({ "Roblox-Session-Id", sessionIdJson.dump() });
    headers.insert({ "Roblox-Place-Id", PlaceId });
    headers.insert({ "Roblox-Game-Id", GameId });
    headers.insert({ "ExploitIdentifier", "YuB-X" });
    headers.insert({ "Accept", "*/*" });

    return HelpFuncs::YieldExecution(L, [UrlStr, headers, CacheOrNot]() -> std::function<int(lua_State*)> {
        cpr::Response Result;
        try {
            Result = cpr::Get(cpr::Url{ UrlStr }, cpr::Header(headers));
        }
        catch (const std::exception& ex) {
            std::string err = std::format("HttpGet crashed: {}", ex.what());
            return [err](lua_State* L) -> int {
                lua_pushstring(L, err.c_str());
                return 1;
                };
        }
        catch (...) {
            return [](lua_State* L) -> int {
                lua_pushstring(L, "HttpGet crashed: unknown exception");
                return 1;
                };
        }

        return [Result, UrlStr, CacheOrNot](lua_State* L) -> int {
            if (Result.error.code != cpr::ErrorCode::OK) {
                auto err = std::format("HttpGet failed: {} (Code {})", Result.error.message, Result.status_code);
                lua_pushstring(L, err.c_str());
                return 1;
            }

            if (Result.status_code != 200) {
                auto err = std::format("HttpGet returned status {}: {}", Result.status_code, Result.error.message);
                lua_pushstring(L, err.c_str());
                return 1;
            }

            if (CacheOrNot)
                Caching[UrlStr] = Result.text;

            lua_pushlstring(L, Result.text.data(), Result.text.size());
            return 1;
            };
        });
}


namespace GameHooks {
    lua_CFunction NamecallClosureBefore;
    lua_CFunction ClosureIndexBefore;

    std::vector<const char*> dangerousFunctions = {
        "OpenVideosFolder", "OpenScreenshotsFolder", "GetRobuxBalance", "PerformPurchase",
        "PromptBundlePurchase", "PromptNativePurchase", "PromptProductPurchase", "PromptPurchase",
        "PromptThirdPartyPurchase", "Publish", "GetMessageId", "OpenBrowserWindow", "RequestInternal",
        "ExecuteJavaScript", "ToggleRecording", "TakeScreenshot", "HttpRequestAsync", "GetLast",
        "SendCommand", "GetAsync", "GetAsyncFullUrl", "RequestAsync", "MakeRequest",
        "AddCoreScriptLocal", "SaveScriptProfilingData", "GetUserSubscriptionDetailsInternalAsync",
        "GetUserSubscriptionStatusAsync", "PerformBulkPurchase", "PerformCancelSubscription",
        "PerformPurchaseV2", "PerformSubscriptionPurchase", "PerformSubscriptionPurchaseV2",
        "PrepareCollectiblesPurchase", "PromptBulkPurchase", "PromptCancelSubscription",
        "PromptCollectiblesPurchase", "PromptGamePassPurchase", "PromptNativePurchaseWithLocalPlayer",
        "PromptPremiumPurchase", "PromptRobloxPurchase", "PromptSubscriptionPurchase",
        "ReportAbuse", "ReportAbuseV3", "ReturnToJavaScript", "OpenNativeOverlay",
        "OpenWeChatAuthWindow", "EmitHybridEvent", "OpenUrl", "PostAsync", "PostAsyncFullUrl",
        "RequestLimitedAsync", "Load", "CaptureScreenshot", "CreatePostAsync", "DeleteCapture",
        "DeleteCapturesAsync", "GetCaptureFilePathAsync", "SaveCaptureToExternalStorage",
        "SaveCapturesToExternalStorageAsync", "GetCaptureUploadDataAsync", "RetrieveCaptures",
        "SaveScreenshotCapture", "Call", "GetProtocolMethodRequestMessageId",
        "GetProtocolMethodResponseMessageId", "PublishProtocolMethodRequest",
        "PublishProtocolMethodResponse", "Subscribe", "SubscribeToProtocolMethodRequest",
        "SubscribeToProtocolMethodResponse", "GetDeviceIntegrityToken", "GetDeviceIntegrityTokenYield",
        "NoPromptCreateOutfit", "NoPromptDeleteOutfit", "NoPromptRenameOutfit", "NoPromptSaveAvatar",
        "NoPromptSaveAvatarThumbnailCustomization", "NoPromptSetFavorite", "NoPromptUpdateOutfit",
        "PerformCreateOutfitWithDescription", "PerformRenameOutfit", "PerformSaveAvatarWithDescription",
        "PerformSetFavorite", "PerformUpdateOutfit", "PromptCreateOutfit", "PromptDeleteOutfit",
        "PromptRenameOutfit", "PromptSaveAvatar", "PromptSetFavorite", "PromptUpdateOutfit"
    };

    int getobjects(lua_State* L) {
        luaL_trimstack(L, 2);
        luaL_checktype(L, 1, LUA_TUSERDATA);
        luaL_checktype(L, 2, LUA_TSTRING);

        lua_getglobal(L, "game");
        lua_getfield(L, -1, "GetService");
        lua_pushvalue(L, -2);
        lua_pushstring(L, "InsertService");
        lua_call(L, 2, 1);
        lua_remove(L, -2);

        lua_getfield(L, -1, "LoadLocalAsset");

        lua_pushvalue(L, -2);
        lua_pushvalue(L, 2);
        lua_pcall(L, 2, 1, 0);

        if (lua_type(L, -1) == LUA_TSTRING) {
            luaL_error(L, lua_tostring(L, -1));
        }

        lua_createtable(L, 1, 0);
        lua_pushvalue(L, -2);
        lua_rawseti(L, -2, 1);

        lua_remove(L, -3);
        lua_remove(L, -2);

        return 1;
    }

    inline bool IsOurThread(lua_State* L) {
        return L->userdata->Capabilities & (1ull << 48ull) == (1ull << 48ull);
    }

    int IndexHook(lua_State* L) {
        if (IsOurThread(L)) {
            std::string key = lua_isstring(L, 2) ? lua_tostring(L, 2) : "";
            for (const char* func : dangerousFunctions) {
                if (key == func) {
                    luaL_error(L, "Function '%s' has been disabled for security reasons.", func);
                    return 0;
                }
            }
            if (L->userdata->Script.expired()) {
                if (key == "HttpGet" || key == "HttpGetAsync") {
                    lua_pushcclosure(L, HttpGet, nullptr, 0);
                    return 1;
                }
                else if (key == "GetObjects") {
                    lua_pushcclosure(L, getobjects, nullptr, 0);
                    return 1;
                }
            }
        }

        return ClosureIndexBefore(L);
    };

    int NamecallHook(lua_State* L) {
        if (IsOurThread(L)) {
            std::string key = L->namecall->data;
            for (const char* func : dangerousFunctions) {
                if (key == func) {
                    luaL_error(L, "Function '%s' has been disabled for security reasons.", func);
                    return 0;
                }
            }
            if (L->userdata->Script.expired()) {
                if (key == "HttpGet" || key == "HttpGetAsync")
                    return HttpGet(L);
                else if (key == "GetObjects")
                    return getobjects(L);
            }
        }
        return NamecallClosureBefore(L);
    };

    void InitializeHooks(lua_State* L) {
        int StackBefore = lua_gettop(L);
        lua_getglobal(L, "game");
        luaL_getmetafield(L, -1, "__index");
        if (lua_type(L, -1) == LUA_TFUNCTION || lua_type(L, -1) == LUA_TLIGHTUSERDATA) {
            Closure* ClosureIndex = clvalue(luaA_toobject(L, -1));
            ClosureIndexBefore = ClosureIndex->c.f;
            ClosureIndex->c.f = IndexHook;
        }
        lua_pop(L, 1);

        luaL_getmetafield(L, -1, "__namecall");
        if (lua_type(L, -1) == LUA_TFUNCTION || lua_type(L, -1) == LUA_TLIGHTUSERDATA) {
            Closure* NamecallClosure = clvalue(luaA_toobject(L, -1));
            NamecallClosureBefore = NamecallClosure->c.f;
            NamecallClosure->c.f = NamecallHook;
        }
        lua_pop(L, 1);

        lua_settop(L, StackBefore);
    }
}


void Http::Register(lua_State* L)
{
    GameHooks::InitializeHooks(L);
    lua_newtable(L);
    RegisterMember(L, HttpGet, "get");
    lua_setglobal(L, "http");

    RegisterFunction(L, HttpGet, "httpget");
    lua_setglobal(L, "httpget");
}