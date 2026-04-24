-- Example Luau script tool
-- This script defines a function that can be called as a tool by the AI Agent

-- Simple calculator function
function eval_expr(expr: string): number
    -- Use loadstring to safely evaluate mathematical expressions
    local fn = loadstring("return " .. expr)
    if fn then
        local ok, result = pcall(fn)
        if ok then
            return result
        else
            error("Evaluation error: " .. tostring(result))
        end
    else
        error("Invalid expression: " .. expr)
    end
end

-- String manipulation tool
function reverse_string(s: string): string
    local result = ""
    for i = #s, 1, -1 do
        result = result .. s:sub(i, i)
    end
    return result
end

-- JSON helper: convert table to JSON-like string
function to_json_string(t: any): string
    if type(t) == "table" then
        local parts = {}
        local is_array = true
        for k, v in t do
            if type(k) ~= "number" then
                is_array = false
            end
            table.insert(parts, to_json_string(v))
        end
        if is_array then
            return "[" .. table.concat(parts, ",") .. "]"
        else
            return "{" .. table.concat(parts, ",") .. "}"
        end
    elseif type(t) == "string" then
        return '"' .. t .. '"'
    elseif type(t) == "number" then
        return tostring(t)
    elseif type(t) == "boolean" then
        return t and "true" or "false"
    else
        return "null"
    end
end

return {
    eval_expr = eval_expr,
    reverse_string = reverse_string,
    to_json_string = to_json_string,
}
