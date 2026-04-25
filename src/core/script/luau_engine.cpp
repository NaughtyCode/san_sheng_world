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
        LOG_ERROR("LuauEngine: failed to create Lua state");
        return;
    }

    // 仅开启安全的库：base, math, string, table, bit32, utf8, buffer, coroutine
    // 不使用 luaL_sandbox() 因为它会将全局表设为只读，阻止脚本加载。
    // 改为手动沙箱化：移除危险的全局变量和库。
    luaL_openlibs(L_);

    // ---------- 移除危险的库 ----------
    // os 库可执行系统命令，必须在沙箱中移除
    lua_pushnil(L_);
    lua_setglobal(L_, "os");

    // debug 库可操作元表和上值，存在安全隐患
    lua_pushnil(L_);
    lua_setglobal(L_, "debug");

    // io 库可读写文件系统，必须在沙箱中移除
    lua_pushnil(L_);
    lua_setglobal(L_, "io");

    // ---------- 移除危险的全局函数 ----------
    // loadfile / dofile 可加载任意 Lua 源文件
    lua_pushnil(L_);
    lua_setglobal(L_, "loadfile");

    lua_pushnil(L_);
    lua_setglobal(L_, "dofile");

    // require 可加载任意模块，移除后用自定义的受限加载器替代
    lua_pushnil(L_);
    lua_setglobal(L_, "require");

    // ---------- 注册新日志 API 到 Luau 虚拟机 ----------
    // Luau 脚本可通过 log_debug / log_info / log_warn / log_error 输出日志
    register_log_api();

    LOG_INFO("LuauEngine: initialized with sandbox and log API");
}

// ==========================================================================
// 注册日志 API 到 Luau 虚拟机
// 在 Luau 脚本中可以使用以下函数：
//   log_debug("message with {}", arg)  -- 调试日志
//   log_info("message with {}", arg)   -- 信息日志
//   log_warn("message with {}", arg)   -- 警告日志
//   log_error("message with {}", arg)  -- 错误日志
// 每个函数接受一个字符串参数，内部委托给 C++ 的 LOG_* 宏。
// ==========================================================================
void LuauEngine::register_log_api() {
    if (!L_) return;

    // ---------- log_debug ----------
    lua_pushcfunction(L_, [](lua_State* L) -> int {
        // 获取 Luau 调用栈上的第一个参数（字符串）
        const char* msg = luaL_checkstring(L, 1);
        LOG_DEBUG("[Luau] {}", msg);
        return 0;
    }, "log_debug");
    lua_setglobal(L_, "log_debug");

    // ---------- log_info ----------
    lua_pushcfunction(L_, [](lua_State* L) -> int {
        const char* msg = luaL_checkstring(L, 1);
        LOG_INFO("[Luau] {}", msg);
        return 0;
    }, "log_info");
    lua_setglobal(L_, "log_info");

    // ---------- log_warn ----------
    lua_pushcfunction(L_, [](lua_State* L) -> int {
        const char* msg = luaL_checkstring(L, 1);
        LOG_WARN("[Luau] {}", msg);
        return 0;
    }, "log_warn");
    lua_setglobal(L_, "log_warn");

    // ---------- log_error ----------
    lua_pushcfunction(L_, [](lua_State* L) -> int {
        const char* msg = luaL_checkstring(L, 1);
        LOG_ERROR("[Luau] {}", msg);
        return 0;
    }, "log_error");
    lua_setglobal(L_, "log_error");

    LOG_DEBUG("LuauEngine: registered log API functions");
}

void LuauEngine::load_script(const std::string& path) {
    if (!L_) {
        LOG_ERROR("LuauEngine: not initialized");
        return;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("LuauEngine: cannot open script: {}", path);
        return;
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    load_string(path, oss.str());
}

void LuauEngine::load_string(const std::string& name, const std::string& source) {
    if (!L_) {
        LOG_ERROR("LuauEngine: not initialized");
        return;
    }

    // 编译 Luau 源码为字节码（优化级别 1，调试级别 1）
    lua_CompileOptions options = {};
    options.optimizationLevel = 1;
    options.debugLevel = 1;

    size_t bytecode_size = 0;
    char* bytecode = luau_compile(source.c_str(), source.size(), &options, &bytecode_size);

    if (!bytecode) {
        LOG_ERROR("LuauEngine: compilation failed for {}", name);
        return;
    }

    // 加载编译后的字节码到 Luau 虚拟机
    int result = luau_load(L_, name.c_str(), bytecode, bytecode_size, 0);
    std::free(bytecode);

    if (result != 0) {
        const char* err = lua_tostring(L_, -1);
        LOG_ERROR("LuauEngine: load failed for {}: {}", name,
                   err ? err : "unknown error");
        lua_pop(L_, 1);
        return;
    }

    // 执行加载的字节码块以注册函数定义
    int status = lua_pcall(L_, 0, 0, 0);
    if (status != LUA_OK && status != 0) {
        const char* err = lua_tostring(L_, -1);
        LOG_ERROR("LuauEngine: execution failed for {}: {}", name,
                   err ? err : "unknown error");
        lua_pop(L_, 1);
        return;
    }

    LOG_INFO("LuauEngine: loaded script \"{}\"", name);
}

json LuauEngine::call_function(const std::string& name, const json& args) {
    if (!L_) {
        LOG_ERROR("LuauEngine: not initialized");
        return json::object();
    }

    // 在全局表中查找目标函数
    lua_getglobal(L_, name.c_str());
    if (!lua_isfunction(L_, -1)) {
        LOG_ERROR("LuauEngine: function \"{}\" not found", name);
        lua_pop(L_, 1);
        return json{{"error", "function not found"}};
    }

    // 将 JSON 参数压入 Lua 栈
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

    // 调用 Luau 函数（保护模式）
    int status = lua_pcall(L_, nargs, 1, 0);
    if (status != LUA_OK && status != 0) {
        const char* err = lua_tostring(L_, -1);
        std::string error_msg = err ? err : "unknown error";
        LOG_ERROR("LuauEngine: call to \"{}\" failed: {}", name, error_msg);
        lua_pop(L_, 1);
        return json{{"error", error_msg}};
    }

    // 提取返回值并转换为 JSON
    json result = pop_json_value();
    return result;
}

json LuauEngine::evaluate_expression(const std::string& expr) {
    if (!L_) {
        LOG_ERROR("LuauEngine: not initialized");
        return json{{"error", "engine not initialized"}};
    }

    if (expr.empty()) {
        return json{{"error", "empty expression"}};
    }

    // 构造 "return <expr>" 语句，编译为字节码后安全执行
    std::string code = "return " + expr;

    lua_CompileOptions options = {};
    options.optimizationLevel = 1;
    options.debugLevel = 1;

    size_t bytecode_size = 0;
    char* bytecode = luau_compile(code.c_str(), code.size(), &options, &bytecode_size);
    if (!bytecode) {
        LOG_ERROR("LuauEngine: expression compilation failed: {}", expr);
        return json{{"error", "invalid expression: " + expr}};
    }

    int load_result = luau_load(L_, "=eval", bytecode, bytecode_size, 0);
    std::free(bytecode);

    if (load_result != 0) {
        const char* err = lua_tostring(L_, -1);
        std::string err_str = err ? err : "unknown error";
        LOG_ERROR("LuauEngine: expression load failed: {}", err_str);
        lua_pop(L_, 1);
        return json{{"error", err_str}};
    }

    // pcall 执行字节码，接收 1 个返回值
    int status = lua_pcall(L_, 0, 1, 0);
    if (status != LUA_OK && status != 0) {
        const char* err = lua_tostring(L_, -1);
        std::string err_str = err ? err : "unknown error";
        LOG_ERROR("LuauEngine: expression execution failed: {}", err_str);
        lua_pop(L_, 1);
        return json{{"error", err_str}};
    }

    // 提取返回值并转换为 JSON
    json result = pop_json_value();
    return result;
}

void LuauEngine::register_cpp_function(const std::string& name,
                                        std::function<json(const json&)> handler) {
    if (!L_) return;

    // 将 C++ handler 存储到堆上，通过 upvalue 传递到 Lua 闭包
    auto* handler_ptr = new std::function<json(const json&)>(std::move(handler));

    // 创建 C 闭包：当 Luau 调用该函数时，此 lambda 作为 bridge
    auto cfunc = [](lua_State* L) -> int {
        // 从 upvalue 中取回 C++ handler 指针
        auto* h = static_cast<std::function<json(const json&)>*>(
            lua_tolightuserdata(L, lua_upvalueindex(1)));

        // 收集 Luau 调用参数并转换为 JSON 数组
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
            // 将 C++ 异常转换为 Lua 错误
            lua_pushstring(L, e.what());
            lua_error(L);
            return 0;
        }

        // 将 JSON 结果序列化为字符串返回给 Luau
        std::string result_str = result.dump();
        lua_pushstring(L, result_str.c_str());
        return 1;
    };

    // 将 handler 指针作为 lightuserdata 压入，然后创建闭包
    lua_pushlightuserdata(L_, handler_ptr);
    lua_pushcclosure(L_, cfunc, name.c_str(), 1);
    lua_setglobal(L_, name.c_str());

    LOG_INFO("LuauEngine: registered C++ function \"{}\"", name);
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
            // 递归遍历 Lua table，转换为 JSON object
            result = json::object();
            lua_pushnil(L_);
            while (lua_next(L_, -2) != 0) {
                // Key 在 -2 位置，value 在 -1 位置
                std::string key;
                if (lua_type(L_, -2) == LUA_TSTRING) {
                    key = lua_tostring(L_, -2);
                } else if (lua_type(L_, -2) == LUA_TNUMBER) {
                    key = std::to_string(static_cast<int>(lua_tonumber(L_, -2)));
                }
                json val = pop_json_value();
                lua_pop(L_, 1); // 弹出 value，保留 key 供下次迭代

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
