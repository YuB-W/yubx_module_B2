// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#include "Builtins.h"

#include "Luau/Bytecode.h"
#include "Luau/Compiler.h"
#include "Luau/Lexer.h"

#include <array>

namespace Luau
{
namespace Compile
{

Builtin getBuiltin(AstExpr* node, const DenseHashMap<AstName, Global>& globals, const DenseHashMap<AstLocal*, Variable>& variables)
{
    if (AstExprLocal* expr = node->as<AstExprLocal>())
    {
        const Variable* v = variables.find(expr->local);

        return v && !v->written && v->init ? getBuiltin(v->init, globals, variables) : Builtin();
    }
    else if (AstExprIndexName* expr = node->as<AstExprIndexName>())
    {
        if (AstExprGlobal* object = expr->expr->as<AstExprGlobal>())
        {
            return getGlobalState(globals, object->name) == Global::Default ? Builtin{object->name, expr->index} : Builtin();
        }
        else
        {
            return Builtin();
        }
    }
    else if (AstExprGlobal* expr = node->as<AstExprGlobal>())
    {
        return getGlobalState(globals, expr->name) == Global::Default ? Builtin{AstName(), expr->name} : Builtin();
    }
    else
    {
        return Builtin();
    }
}

static int getBuiltinFunctionId(const Builtin& builtin, const CompileOptions& options)
{
    if (builtin.isGlobal("assert"))
        return LBF_ASSERT;

    if (builtin.isGlobal("type"))
        return LBF_TYPE;

    if (builtin.isGlobal("typeof"))
        return LBF_TYPEOF;

    if (builtin.isGlobal("rawset"))
        return LBF_RAWSET;
    if (builtin.isGlobal("rawget"))
        return LBF_RAWGET;
    if (builtin.isGlobal("rawequal"))
        return LBF_RAWEQUAL;
    if (builtin.isGlobal("rawlen"))
        return LBF_RAWLEN;

    if (builtin.isGlobal("unpack"))
        return LBF_TABLE_UNPACK;

    if (builtin.isGlobal("select"))
        return LBF_SELECT_VARARG;

    if (builtin.isGlobal("getmetatable"))
        return LBF_GETMETATABLE;
    if (builtin.isGlobal("setmetatable"))
        return LBF_SETMETATABLE;

    if (builtin.isGlobal("tonumber"))
        return LBF_TONUMBER;
    if (builtin.isGlobal("tostring"))
        return LBF_TOSTRING;

    if (builtin.object == "math")
    {
        if (builtin.method == "abs")
            return LBF_MATH_ABS;
        if (builtin.method == "acos")
            return LBF_MATH_ACOS;
        if (builtin.method == "asin")
            return LBF_MATH_ASIN;
        if (builtin.method == "atan2")
            return LBF_MATH_ATAN2;
        if (builtin.method == "atan")
            return LBF_MATH_ATAN;
        if (builtin.method == "ceil")
            return LBF_MATH_CEIL;
        if (builtin.method == "cosh")
            return LBF_MATH_COSH;
        if (builtin.method == "cos")
            return LBF_MATH_COS;
        if (builtin.method == "deg")
            return LBF_MATH_DEG;
        if (builtin.method == "exp")
            return LBF_MATH_EXP;
        if (builtin.method == "floor")
            return LBF_MATH_FLOOR;
        if (builtin.method == "fmod")
            return LBF_MATH_FMOD;
        if (builtin.method == "frexp")
            return LBF_MATH_FREXP;
        if (builtin.method == "ldexp")
            return LBF_MATH_LDEXP;
        if (builtin.method == "log10")
            return LBF_MATH_LOG10;
        if (builtin.method == "log")
            return LBF_MATH_LOG;
        if (builtin.method == "max")
            return LBF_MATH_MAX;
        if (builtin.method == "min")
            return LBF_MATH_MIN;
        if (builtin.method == "modf")
            return LBF_MATH_MODF;
        if (builtin.method == "pow")
            return LBF_MATH_POW;
        if (builtin.method == "rad")
            return LBF_MATH_RAD;
        if (builtin.method == "sinh")
            return LBF_MATH_SINH;
        if (builtin.method == "sin")
            return LBF_MATH_SIN;
        if (builtin.method == "sqrt")
            return LBF_MATH_SQRT;
        if (builtin.method == "tanh")
            return LBF_MATH_TANH;
        if (builtin.method == "tan")
            return LBF_MATH_TAN;
        if (builtin.method == "clamp")
            return LBF_MATH_CLAMP;
        if (builtin.method == "sign")
            return LBF_MATH_SIGN;
        if (builtin.method == "round")
            return LBF_MATH_ROUND;
        if (builtin.method == "lerp")
            return LBF_MATH_LERP;
    }

    if (builtin.object == "bit32")
    {
        if (builtin.method == "arshift")
            return LBF_BIT32_ARSHIFT;
        if (builtin.method == "band")
            return LBF_BIT32_BAND;
        if (builtin.method == "bnot")
            return LBF_BIT32_BNOT;
        if (builtin.method == "bor")
            return LBF_BIT32_BOR;
        if (builtin.method == "bxor")
            return LBF_BIT32_BXOR;
        if (builtin.method == "btest")
            return LBF_BIT32_BTEST;
        if (builtin.method == "extract")
            return LBF_BIT32_EXTRACT;
        if (builtin.method == "lrotate")
            return LBF_BIT32_LROTATE;
        if (builtin.method == "lshift")
            return LBF_BIT32_LSHIFT;
        if (builtin.method == "replace")
            return LBF_BIT32_REPLACE;
        if (builtin.method == "rrotate")
            return LBF_BIT32_RROTATE;
        if (builtin.method == "rshift")
            return LBF_BIT32_RSHIFT;
        if (builtin.method == "countlz")
            return LBF_BIT32_COUNTLZ;
        if (builtin.method == "countrz")
            return LBF_BIT32_COUNTRZ;
        if (builtin.method == "byteswap")
            return LBF_BIT32_BYTESWAP;
    }

    if (builtin.object == "string")
    {
        if (builtin.method == "byte")
            return LBF_STRING_BYTE;
        if (builtin.method == "char")
            return LBF_STRING_CHAR;
        if (builtin.method == "len")
            return LBF_STRING_LEN;
        if (builtin.method == "sub")
            return LBF_STRING_SUB;
    }

    if (builtin.object == "table")
    {
        if (builtin.method == "insert")
            return LBF_TABLE_INSERT;
        if (builtin.method == "unpack")
            return LBF_TABLE_UNPACK;
    }

    if (builtin.object == "buffer")
    {
        if (builtin.method == "readi8")
            return LBF_BUFFER_READI8;
        if (builtin.method == "readu8")
            return LBF_BUFFER_READU8;
        if (builtin.method == "writei8" || builtin.method == "writeu8")
            return LBF_BUFFER_WRITEU8;
        if (builtin.method == "readi16")
            return LBF_BUFFER_READI16;
        if (builtin.method == "readu16")
            return LBF_BUFFER_READU16;
        if (builtin.method == "writei16" || builtin.method == "writeu16")
            return LBF_BUFFER_WRITEU16;
        if (builtin.method == "readi32")
            return LBF_BUFFER_READI32;
        if (builtin.method == "readu32")
            return LBF_BUFFER_READU32;
        if (builtin.method == "writei32" || builtin.method == "writeu32")
            return LBF_BUFFER_WRITEU32;
        if (builtin.method == "readf32")
            return LBF_BUFFER_READF32;
        if (builtin.method == "writef32")
            return LBF_BUFFER_WRITEF32;
        if (builtin.method == "readf64")
            return LBF_BUFFER_READF64;
        if (builtin.method == "writef64")
            return LBF_BUFFER_WRITEF64;
    }

    if (builtin.object == "vector")
    {
        if (builtin.method == "create")
            return LBF_VECTOR;
        if (builtin.method == "magnitude")
            return LBF_VECTOR_MAGNITUDE;
        if (builtin.method == "normalize")
            return LBF_VECTOR_NORMALIZE;
        if (builtin.method == "cross")
            return LBF_VECTOR_CROSS;
        if (builtin.method == "dot")
            return LBF_VECTOR_DOT;
        if (builtin.method == "floor")
            return LBF_VECTOR_FLOOR;
        if (builtin.method == "ceil")
            return LBF_VECTOR_CEIL;
        if (builtin.method == "abs")
            return LBF_VECTOR_ABS;
        if (builtin.method == "sign")
            return LBF_VECTOR_SIGN;
        if (builtin.method == "clamp")
            return LBF_VECTOR_CLAMP;
        if (builtin.method == "min")
            return LBF_VECTOR_MIN;
        if (builtin.method == "max")
            return LBF_VECTOR_MAX;
    }

    if (options.vectorCtor)
    {
        if (options.vectorLib)
        {
            if (builtin.isMethod(options.vectorLib, options.vectorCtor))
                return LBF_VECTOR;
        }
        else
        {
            if (builtin.isGlobal(options.vectorCtor))
                return LBF_VECTOR;
        }
    }

    return -1;
}

struct BuiltinVisitor : AstVisitor
{
    DenseHashMap<AstExprCall*, int>& result;
    std::array<bool, 256> builtinIsDisabled;

    const DenseHashMap<AstName, Global>& globals;
    const DenseHashMap<AstLocal*, Variable>& variables;

    const CompileOptions& options;
    const AstNameTable& names;

    BuiltinVisitor(
        DenseHashMap<AstExprCall*, int>& result,
        const DenseHashMap<AstName, Global>& globals,
        const DenseHashMap<AstLocal*, Variable>& variables,
        const CompileOptions& options,
        const AstNameTable& names
    )
        : result(result)
        , globals(globals)
        , variables(variables)
        , options(options)
        , names(names)
    {
        builtinIsDisabled.fill(false);

        if (const char* const* ptr = options.disabledBuiltins)
        {
            for (; *ptr; ++ptr)
            {
                if (const char* dot = strchr(*ptr, '.'))
                {
                    AstName library = names.getWithType(*ptr, dot - *ptr).first;
                    AstName name = names.get(dot + 1);

                    if (library.value && name.value && getGlobalState(globals, name) == Global::Default)
                    {
                        Builtin builtin = Builtin{library, name};

                        if (int bfid = getBuiltinFunctionId(builtin, options); bfid >= 0)
                            builtinIsDisabled[bfid] = true;
                    }
                }
                else
                {
                    if (AstName name = names.get(*ptr); name.value && getGlobalState(globals, name) == Global::Default)
                    {
                        Builtin builtin = Builtin{AstName(), name};

                        if (int bfid = getBuiltinFunctionId(builtin, options); bfid >= 0)
                            builtinIsDisabled[bfid] = true;
                    }
                }
            }
        }
    }

    bool visit(AstExprCall* node) override
    {
        Builtin builtin = node->self ? Builtin() : getBuiltin(node->func, globals, variables);
        if (builtin.empty())
            return true;

        int bfid = getBuiltinFunctionId(builtin, options);

        if (bfid >= 0 && builtinIsDisabled[bfid])
            bfid = -1;

        // getBuiltinFunctionId optimistically assumes all select() calls are builtin but actually the second argument must be a vararg
        if (bfid == LBF_SELECT_VARARG && !(node->args.size == 2 && node->args.data[1]->is<AstExprVarargs>()))
            bfid = -1;

        if (bfid >= 0)
            result[node] = bfid;

        return true; // propagate to nested calls
    }
};

void analyzeBuiltins(
    DenseHashMap<AstExprCall*, int>& result,
    const DenseHashMap<AstName, Global>& globals,
    const DenseHashMap<AstLocal*, Variable>& variables,
    const CompileOptions& options,
    AstNode* root,
    const AstNameTable& names
)
{
    BuiltinVisitor visitor{result, globals, variables, options, names};
    root->visit(&visitor);
}

BuiltinInfo getBuiltinInfo(int bfid)
{
    switch (LuauBuiltinFunction(bfid))
    {
    case LBF_NONE:
        return {-1, -1};

    case LBF_ASSERT:
        return {-1, -1}; // assert() returns all values when first value is truthy

    case LBF_MATH_ABS:
    case LBF_MATH_ACOS:
    case LBF_MATH_ASIN:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_MATH_ATAN2:
        return {2, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_MATH_ATAN:
    case LBF_MATH_CEIL:
    case LBF_MATH_COSH:
    case LBF_MATH_COS:
    case LBF_MATH_DEG:
    case LBF_MATH_EXP:
    case LBF_MATH_FLOOR:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_MATH_FMOD:
        return {2, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_MATH_FREXP:
        return {1, 2, BuiltinInfo::Flag_NoneSafe};

    case LBF_MATH_LDEXP:
        return {2, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_MATH_LOG10:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_MATH_LOG:
        return {-1, 1}; // 1 or 2 parameters

    case LBF_MATH_MAX:
    case LBF_MATH_MIN:
        return {-1, 1}; // variadic

    case LBF_MATH_MODF:
        return {1, 2, BuiltinInfo::Flag_NoneSafe};

    case LBF_MATH_POW:
        return {2, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_MATH_RAD:
    case LBF_MATH_SINH:
    case LBF_MATH_SIN:
    case LBF_MATH_SQRT:
    case LBF_MATH_TANH:
    case LBF_MATH_TAN:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_BIT32_ARSHIFT:
        return {2, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_BIT32_BAND:
        return {-1, 1}; // variadic

    case LBF_BIT32_BNOT:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_BIT32_BOR:
    case LBF_BIT32_BXOR:
    case LBF_BIT32_BTEST:
        return {-1, 1}; // variadic

    case LBF_BIT32_EXTRACT:
        return {-1, 1}; // 2 or 3 parameters

    case LBF_BIT32_LROTATE:
    case LBF_BIT32_LSHIFT:
        return {2, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_BIT32_REPLACE:
        return {-1, 1}; // 3 or 4 parameters

    case LBF_BIT32_RROTATE:
    case LBF_BIT32_RSHIFT:
        return {2, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_TYPE:
        return {1, 1};

    case LBF_STRING_BYTE:
        return {-1, -1}; // 1, 2 or 3 parameters

    case LBF_STRING_CHAR:
        return {-1, 1}; // variadic

    case LBF_STRING_LEN:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_TYPEOF:
        return {1, 1};

    case LBF_STRING_SUB:
        return {-1, 1}; // 2 or 3 parameters

    case LBF_MATH_CLAMP:
        return {3, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_MATH_SIGN:
    case LBF_MATH_ROUND:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_RAWSET:
        return {3, 1};

    case LBF_RAWGET:
    case LBF_RAWEQUAL:
        return {2, 1};

    case LBF_TABLE_INSERT:
        return {-1, 0}; // 2 or 3 parameters

    case LBF_TABLE_UNPACK:
        return {-1, -1}; // 1, 2 or 3 parameters

    case LBF_VECTOR:
        return {-1, 1}; // 3 or 4 parameters in some configurations

    case LBF_BIT32_COUNTLZ:
    case LBF_BIT32_COUNTRZ:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_SELECT_VARARG:
        return {-1, -1}; // variadic

    case LBF_RAWLEN:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_BIT32_EXTRACTK:
        return {3, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_GETMETATABLE:
        return {1, 1};

    case LBF_SETMETATABLE:
        return {2, 1};

    case LBF_TONUMBER:
        return {-1, 1}; // 1 or 2 parameters

    case LBF_TOSTRING:
        return {1, 1};

    case LBF_BIT32_BYTESWAP:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_BUFFER_READI8:
    case LBF_BUFFER_READU8:
    case LBF_BUFFER_READI16:
    case LBF_BUFFER_READU16:
    case LBF_BUFFER_READI32:
    case LBF_BUFFER_READU32:
    case LBF_BUFFER_READF32:
    case LBF_BUFFER_READF64:
        return {2, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_BUFFER_WRITEU8:
    case LBF_BUFFER_WRITEU16:
    case LBF_BUFFER_WRITEU32:
    case LBF_BUFFER_WRITEF32:
    case LBF_BUFFER_WRITEF64:
        return {3, 0, BuiltinInfo::Flag_NoneSafe};

    case LBF_VECTOR_MAGNITUDE:
    case LBF_VECTOR_NORMALIZE:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};
    case LBF_VECTOR_CROSS:
    case LBF_VECTOR_DOT:
        return {2, 1, BuiltinInfo::Flag_NoneSafe};
    case LBF_VECTOR_FLOOR:
    case LBF_VECTOR_CEIL:
    case LBF_VECTOR_ABS:
    case LBF_VECTOR_SIGN:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};
    case LBF_VECTOR_CLAMP:
        return {3, 1, BuiltinInfo::Flag_NoneSafe};
    case LBF_VECTOR_MIN:
    case LBF_VECTOR_MAX:
        return {-1, 1}; // variadic

    case LBF_MATH_LERP:
        return {3, 1, BuiltinInfo::Flag_NoneSafe};
    }

    LUAU_UNREACHABLE();
}

} // namespace Compile
} // namespace Luau
