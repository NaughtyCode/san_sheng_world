#include "script/luau_engine.hpp"
#include "logger.hpp"

#include "lua.h"
#include "lualib.h"
#include "luacode.h"

#include <fstream>
#include <sstream>

namespace agent {

LuauEngine::LuauEngine() = default;

LuauEngine::~LuauEngine() {
    if (L_) {
        lua_close(L_);
    }
}

void LuauEngine::init() {
    L_ = luaL_newstate();
    if (!L_) {
        Logger::instance().error("LuauEngine: failed to create Lua state");
        return;
    }

    // Open only safe libraries: base, math, string, table, bit32, utf8, buffer, coroutine
    // Do NOT use luaL_sandbox() because it makes the global table read-only,
    // preventing script loading. Instead, manually sandbox.
    luaL_openlibs(L_);

    // Remove dangerous globals and libraries
    lua_pushnil(L_);
    lua_setglobal(L_, "os");

    lua_pushnil(L_);
    lua_setglobal(L_, "debug");

    lua_pushnil(L_);
    lua_setglobal(L_, "io");

    // Remove dangerous base globals
    lua_pushnil(L_);
    lua_setglobal(L_, "loadfile");

    lua_pushnil(L_);
    lua_setglobal(L_, "dofile");

    lua_pushnil(L_);
    lua_setglobal(L_, "require");

    Logger::instance().info("LuauEngine: initialized with sandbox");
}

void LuauEngine::load_script(const std::string& path) {
    if (!L_) {
        Logger::instance().error("LuauEngine: not initialized");
        return;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        Logger::instance().error("LuauEngine: cannot open script: %s", path.c_str());
        return;
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    load_string(path, oss.str());
}

void LuauEngine::load_string(const std::string& name, const std::string& source) {
    if (!L_) {
        Logger::instance().error("LuauEngine: not initialized");
        return;
    }

    lua_CompileOptions options = {};
    options.optimizationLevel = 1;
    options.debugLevel = 1;

    size_t bytecode_size = 0;
    char* bytecode = luau_compile(source.c_str(), source.size(), &options, &bytecode_size);

    if (!bytecode) {
        Logger::instance().error("LuauEngine: compilation failed for %s", name.c_str());
        return;
    }

    int result = luau_load(L_, name.c_str(), bytecode, bytecode_size, 0);
    std::free(bytecode);

    if (result != 0) {
        const char* err = lua_tostring(L_, -1);
        Logger::instance().error("LuauEngine: load failed for %s: %s", name.c_str(),
                                  err ? err : "unknown error");
        lua_pop(L_, 1);
        return;
    }

    // Execute the chunk to define functions
    int status = lua_pcall(L_, 0, 0, 0);
    if (status != LUA_OK && status != 0) {
        const char* err = lua_tostring(L_, -1);
        Logger::instance().error("LuauEngine: execution failed for %s: %s", name.c_str(),
                                  err ? err : "unknown error");
        lua_pop(L_, 1);
        return;
    }

    Logger::instance().info("LuauEngine: loaded script \"%s\"", name.c_str());
}

json LuauEngine::call_function(const std::string& name, const json& args) {
    if (!L_) {
        Logger::instance().error("LuauEngine: not initialized");
        return json::object();
    }

    // Push function onto stack
    lua_getglobal(L_, name.c_str());
    if (!lua_isfunction(L_, -1)) {
        Logger::instance().error("LuauEngine: function \"%s\" not found", name.c_str());
        lua_pop(L_, 1);
        return json{{"error", "function not found"}};
    }

    // Push arguments
    int nargs = 0;
    if (args.is_array()) {
        for (const auto& arg : args) {
            push_json_value(arg);
            ++nargs;
        }
    } else if (!args.is_null()) {
        push_json_value(args);
        nargs = 1;
    }

    // Call function
    int status = lua_pcall(L_, nargs, 1, 0);
    if (status != LUA_OK && status != 0) {
        const char* err = lua_tostring(L_, -1);
        std::string error_msg = err ? err : "unknown error";
        Logger::instance().error("LuauEngine: call to \"%s\" failed: %s", name.c_str(),
                                  error_msg.c_str());
        lua_pop(L_, 1);
        return json{{"error", error_msg}};
    }

    // Extract return value
    json result = pop_json_value();
    return result;
}

void LuauEngine::register_cpp_function(const std::string& name,
                                        std::function<json(const json&)> handler) {
    if (!L_) return;

    // Store handler in registry for later retrieval
    auto* handler_ptr = new std::function<json(const json&)>(std::move(handler));

    auto cfunc = [](lua_State* L) -> int {
        auto* h = static_cast<std::function<json(const json&)>*>(
            lua_tolightuserdata(L, lua_upvalueindex(1)));

        // Collect arguments from the stack as JSON
        json args = json::array();
        int nargs = lua_gettop(L);
        for (int i = 1; i <= nargs; ++i) {
            int t = lua_type(L, i);
            switch (t) {
                case LUA_TNIL:
                    args.push_back(nullptr);
                    break;
                case LUA_TBOOLEAN:
                    args.push_back(lua_toboolean(L, i) != 0);
                    break;
                case LUA_TNUMBER:
                    args.push_back(lua_tonumber(L, i));
                    break;
                case LUA_TSTRING: {
                    size_t len;
                    const char* s = lua_tolstring(L, i, &len);
                    args.push_back(std::string(s, len));
                    break;
                }
                default:
                    args.push_back(nullptr);
                    break;
            }
        }

        json result;
        try {
            result = (*h)(args);
        } catch (const std::exception& e) {
            lua_pushstring(L, e.what());
            lua_error(L);
            return 0;
        }

        // Push result as a JSON string
        std::string result_str = result.dump();
        lua_pushstring(L, result_str.c_str());
        return 1;
    };

    lua_pushlightuserdata(L_, handler_ptr);
    lua_pushcclosure(L_, cfunc, name.c_str(), 1);
    lua_setglobal(L_, name.c_str());

    Logger::instance().info("LuauEngine: registered C++ function \"%s\"", name.c_str());
}

void LuauEngine::push_json_value(const json& value) {
    switch (value.type()) {
        case json::value_t::null:
            lua_pushnil(L_);
            break;
        case json::value_t::boolean:
            lua_pushboolean(L_, value.get<bool>() ? 1 : 0);
            break;
        case json::value_t::number_integer:
        case json::value_t::number_unsigned:
            lua_pushnumber(L_, static_cast<double>(value.get<int64_t>()));
            break;
        case json::value_t::number_float:
            lua_pushnumber(L_, value.get<double>());
            break;
        case json::value_t::string: {
            const std::string& s = value.get_ref<const std::string&>();
            lua_pushstring(L_, s.c_str());
            break;
        }
        case json::value_t::array: {
            lua_createtable(L_, static_cast<int>(value.size()), 0);
            int idx = 1;
            for (const auto& elem : value) {
                push_json_value(elem);
                lua_rawseti(L_, -2, idx++);
            }
            break;
        }
        case json::value_t::object: {
            lua_newtable(L_);
            for (auto it = value.begin(); it != value.end(); ++it) {
                push_json_value(it.value());
                lua_setfield(L_, -2, it.key().c_str());
            }
            break;
        }
        default:
            lua_pushnil(L_);
            break;
    }
}

json LuauEngine::pop_json_value() {
    if (lua_gettop(L_) < 1) return json();

    int t = lua_type(L_, -1);
    json result;

    switch (t) {
        case LUA_TNIL:
            result = nullptr;
            break;
        case LUA_TBOOLEAN:
            result = lua_toboolean(L_, -1) != 0;
            break;
        case LUA_TNUMBER:
            result = lua_tonumber(L_, -1);
            break;
        case LUA_TSTRING: {
            size_t len;
            const char* s = lua_tolstring(L_, -1, &len);
            result = std::string(s, len);
            break;
        }
        case LUA_TTABLE: {
            result = json::object();
            lua_pushnil(L_);
            while (lua_next(L_, -2) != 0) {
                // Key is at -2, value at -1
                std::string key;
                if (lua_type(L_, -2) == LUA_TSTRING) {
                    key = lua_tostring(L_, -2);
                } else if (lua_type(L_, -2) == LUA_TNUMBER) {
                    key = std::to_string(static_cast<int>(lua_tonumber(L_, -2)));
                }
                json val = pop_json_value();
                lua_pop(L_, 1); // pop value so key remains for next iteration

                if (!key.empty()) {
                    result[key] = val;
                }
            }
            break;
        }
        default:
            result = nullptr;
            break;
    }

    lua_pop(L_, 1);
    return result;
}

} // namespace agent
