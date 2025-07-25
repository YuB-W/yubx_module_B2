// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#pragma once

#include "Luau/Location.h"

#include <string>

namespace Luau
{

extern int gCstRttiIndex;

template<typename T>
struct CstRtti
{
    static const int value;
};

template<typename T>
const int CstRtti<T>::value = ++gCstRttiIndex;

#define LUAU_CST_RTTI(Class) \
    static int CstClassIndex() \
    { \
        return CstRtti<Class>::value; \
    }

class CstNode
{
public:
    explicit CstNode(int classIndex)
        : classIndex(classIndex)
    {
    }

    template<typename T>
    bool is() const
    {
        return classIndex == T::CstClassIndex();
    }
    template<typename T>
    T* as()
    {
        return classIndex == T::CstClassIndex() ? static_cast<T*>(this) : nullptr;
    }
    template<typename T>
    const T* as() const
    {
        return classIndex == T::CstClassIndex() ? static_cast<const T*>(this) : nullptr;
    }

    const int classIndex;
};

class CstExprConstantNumber : public CstNode
{
public:
    LUAU_CST_RTTI(CstExprConstantNumber)

    explicit CstExprConstantNumber(const AstArray<char>& value);

    AstArray<char> value;
};

class CstExprConstantString : public CstNode
{
public:
    LUAU_CST_RTTI(CstExprConstantNumber)

    enum QuoteStyle
    {
        QuotedSingle,
        QuotedDouble,
        QuotedRaw,
        QuotedInterp,
    };

    CstExprConstantString(AstArray<char> sourceString, QuoteStyle quoteStyle, unsigned int blockDepth);

    AstArray<char> sourceString;
    QuoteStyle quoteStyle;
    unsigned int blockDepth;
};

class CstExprCall : public CstNode
{
public:
    LUAU_CST_RTTI(CstExprCall)

    CstExprCall(std::optional<Position> openParens, std::optional<Position> closeParens, AstArray<Position> commaPositions);

    std::optional<Position> openParens;
    std::optional<Position> closeParens;
    AstArray<Position> commaPositions;
};

class CstExprIndexExpr : public CstNode
{
public:
    LUAU_CST_RTTI(CstExprIndexExpr)

    CstExprIndexExpr(Position openBracketPosition, Position closeBracketPosition);

    Position openBracketPosition;
    Position closeBracketPosition;
};

class CstExprFunction : public CstNode
{
public:
    LUAU_CST_RTTI(CstExprFunction)

    CstExprFunction();

    Position functionKeywordPosition{0, 0};
    Position openGenericsPosition{0, 0};
    AstArray<Position> genericsCommaPositions;
    Position closeGenericsPosition{0, 0};
    AstArray<Position> argsAnnotationColonPositions;
    AstArray<Position> argsCommaPositions;
    Position varargAnnotationColonPosition{0, 0};
    Position returnSpecifierPosition{0, 0};
};

class CstExprTable : public CstNode
{
public:
    LUAU_CST_RTTI(CstExprTable)

    enum Separator
    {
        Comma,
        Semicolon,
    };

    struct Item
    {
        std::optional<Position> indexerOpenPosition;  // '[', only if Kind == General
        std::optional<Position> indexerClosePosition; // ']', only if Kind == General
        std::optional<Position> equalsPosition;       // only if Kind != List
        std::optional<Separator> separator;           // may be missing for last Item
        std::optional<Position> separatorPosition;
    };

    explicit CstExprTable(const AstArray<Item>& items);

    AstArray<Item> items;
};

// TODO: Shared between unary and binary, should we split?
class CstExprOp : public CstNode
{
public:
    LUAU_CST_RTTI(CstExprOp)

    explicit CstExprOp(Position opPosition);

    Position opPosition;
};

class CstExprTypeAssertion : public CstNode
{
public:
    LUAU_CST_RTTI(CstExprTypeAssertion)

    explicit CstExprTypeAssertion(Position opPosition);

    Position opPosition;
};

class CstExprIfElse : public CstNode
{
public:
    LUAU_CST_RTTI(CstExprIfElse)

    CstExprIfElse(Position thenPosition, Position elsePosition, bool isElseIf);

    Position thenPosition;
    Position elsePosition;
    bool isElseIf;
};

class CstExprInterpString : public CstNode
{
public:
    LUAU_CST_RTTI(CstExprInterpString)

    explicit CstExprInterpString(AstArray<AstArray<char>> sourceStrings, AstArray<Position> stringPositions);

    AstArray<AstArray<char>> sourceStrings;
    AstArray<Position> stringPositions;
};

class CstStatDo : public CstNode
{
public:
    LUAU_CST_RTTI(CstStatDo)

    explicit CstStatDo(Position endPosition);

    Position endPosition;
};

class CstStatRepeat : public CstNode
{
public:
    LUAU_CST_RTTI(CstStatRepeat)

    explicit CstStatRepeat(Position untilPosition);

    Position untilPosition;
};

class CstStatReturn : public CstNode
{
public:
    LUAU_CST_RTTI(CstStatReturn)

    explicit CstStatReturn(AstArray<Position> commaPositions);

    AstArray<Position> commaPositions;
};

class CstStatLocal : public CstNode
{
public:
    LUAU_CST_RTTI(CstStatLocal)

    CstStatLocal(AstArray<Position> varsAnnotationColonPositions, AstArray<Position> varsCommaPositions, AstArray<Position> valuesCommaPositions);

    AstArray<Position> varsAnnotationColonPositions;
    AstArray<Position> varsCommaPositions;
    AstArray<Position> valuesCommaPositions;
};

class CstStatFor : public CstNode
{
public:
    LUAU_CST_RTTI(CstStatFor)

    CstStatFor(Position annotationColonPosition, Position equalsPosition, Position endCommaPosition, std::optional<Position> stepCommaPosition);

    Position annotationColonPosition;
    Position equalsPosition;
    Position endCommaPosition;
    std::optional<Position> stepCommaPosition;
};

class CstStatForIn : public CstNode
{
public:
    LUAU_CST_RTTI(CstStatForIn)

    CstStatForIn(AstArray<Position> varsAnnotationColonPositions, AstArray<Position> varsCommaPositions, AstArray<Position> valuesCommaPositions);

    AstArray<Position> varsAnnotationColonPositions;
    AstArray<Position> varsCommaPositions;
    AstArray<Position> valuesCommaPositions;
};

class CstStatAssign : public CstNode
{
public:
    LUAU_CST_RTTI(CstStatAssign)

    CstStatAssign(AstArray<Position> varsCommaPositions, Position equalsPosition, AstArray<Position> valuesCommaPositions);

    AstArray<Position> varsCommaPositions;
    Position equalsPosition;
    AstArray<Position> valuesCommaPositions;
};

class CstStatCompoundAssign : public CstNode
{
public:
    LUAU_CST_RTTI(CstStatCompoundAssign)

    explicit CstStatCompoundAssign(Position opPosition);

    Position opPosition;
};

class CstStatFunction : public CstNode
{
public:
    LUAU_CST_RTTI(CstStatFunction)

    explicit CstStatFunction(Position functionKeywordPosition);

    Position functionKeywordPosition;
};

class CstStatLocalFunction : public CstNode
{
public:
    LUAU_CST_RTTI(CstStatLocalFunction)

    explicit CstStatLocalFunction(Position localKeywordPosition, Position functionKeywordPosition);

    Position localKeywordPosition;
    Position functionKeywordPosition;
};

class CstGenericType : public CstNode
{
public:
    LUAU_CST_RTTI(CstGenericType)

    CstGenericType(std::optional<Position> defaultEqualsPosition);

    std::optional<Position> defaultEqualsPosition;
};

class CstGenericTypePack : public CstNode
{
public:
    LUAU_CST_RTTI(CstGenericTypePack)

    CstGenericTypePack(Position ellipsisPosition, std::optional<Position> defaultEqualsPosition);

    Position ellipsisPosition;
    std::optional<Position> defaultEqualsPosition;
};

class CstStatTypeAlias : public CstNode
{
public:
    LUAU_CST_RTTI(CstStatTypeAlias)

    CstStatTypeAlias(
        Position typeKeywordPosition,
        Position genericsOpenPosition,
        AstArray<Position> genericsCommaPositions,
        Position genericsClosePosition,
        Position equalsPosition
    );

    Position typeKeywordPosition;
    Position genericsOpenPosition;
    AstArray<Position> genericsCommaPositions;
    Position genericsClosePosition;
    Position equalsPosition;
};

class CstStatTypeFunction : public CstNode
{
public:
    LUAU_CST_RTTI(CstStatTypeFunction)

    CstStatTypeFunction(Position typeKeywordPosition, Position functionKeywordPosition);

    Position typeKeywordPosition;
    Position functionKeywordPosition;
};

class CstTypeReference : public CstNode
{
public:
    LUAU_CST_RTTI(CstTypeReference)

    CstTypeReference(
        std::optional<Position> prefixPointPosition,
        Position openParametersPosition,
        AstArray<Position> parametersCommaPositions,
        Position closeParametersPosition
    );

    std::optional<Position> prefixPointPosition;
    Position openParametersPosition;
    AstArray<Position> parametersCommaPositions;
    Position closeParametersPosition;
};

class CstTypeTable : public CstNode
{
public:
    LUAU_CST_RTTI(CstTypeTable)

    struct Item
    {
        enum struct Kind
        {
            Indexer,
            Property,
            StringProperty,
        };

        Kind kind;
        Position indexerOpenPosition;  // '[', only if Kind != Property
        Position indexerClosePosition; // ']' only if Kind != Property
        Position colonPosition;
        std::optional<CstExprTable::Separator> separator; // may be missing for last Item
        std::optional<Position> separatorPosition;

        CstExprConstantString* stringInfo = nullptr; // only if Kind == StringProperty
        Position stringPosition{0, 0}; // only if Kind == StringProperty
    };

    CstTypeTable(AstArray<Item> items, bool isArray);

    AstArray<Item> items;
    bool isArray = false;
};

class CstTypeFunction : public CstNode
{
public:
    LUAU_CST_RTTI(CstTypeFunction)

    CstTypeFunction(
        Position openGenericsPosition,
        AstArray<Position> genericsCommaPositions,
        Position closeGenericsPosition,
        Position openArgsPosition,
        AstArray<std::optional<Position>> argumentNameColonPositions,
        AstArray<Position> argumentsCommaPositions,
        Position closeArgsPosition,
        Position returnArrowPosition
    );

    Position openGenericsPosition;
    AstArray<Position> genericsCommaPositions;
    Position closeGenericsPosition;
    Position openArgsPosition;
    AstArray<std::optional<Position>> argumentNameColonPositions;
    AstArray<Position> argumentsCommaPositions;
    Position closeArgsPosition;
    Position returnArrowPosition;
};

class CstTypeTypeof : public CstNode
{
public:
    LUAU_CST_RTTI(CstTypeTypeof)

    CstTypeTypeof(Position openPosition, Position closePosition);

    Position openPosition;
    Position closePosition;
};

class CstTypeUnion : public CstNode
{
public:
    LUAU_CST_RTTI(CstTypeUnion)

    CstTypeUnion(std::optional<Position> leadingPosition, AstArray<Position> separatorPositions);

    std::optional<Position> leadingPosition;
    AstArray<Position> separatorPositions;
};

class CstTypeIntersection : public CstNode
{
public:
    LUAU_CST_RTTI(CstTypeIntersection)

    explicit CstTypeIntersection(std::optional<Position> leadingPosition, AstArray<Position> separatorPositions);

    std::optional<Position> leadingPosition;
    AstArray<Position> separatorPositions;
};

class CstTypeSingletonString : public CstNode
{
public:
    LUAU_CST_RTTI(CstTypeSingletonString)

    CstTypeSingletonString(AstArray<char> sourceString, CstExprConstantString::QuoteStyle quoteStyle, unsigned int blockDepth);

    AstArray<char> sourceString;
    CstExprConstantString::QuoteStyle quoteStyle;
    unsigned int blockDepth;
};

class CstTypePackExplicit : public CstNode
{
public:
    LUAU_CST_RTTI(CstTypePackExplicit)

    explicit CstTypePackExplicit();
    explicit CstTypePackExplicit(Position openParenthesesPosition, Position closeParenthesesPosition, AstArray<Position> commaPositions);

    bool hasParentheses;
    Position openParenthesesPosition;
    Position closeParenthesesPosition;
    AstArray<Position> commaPositions;
};

class CstTypePackGeneric : public CstNode
{
public:
    LUAU_CST_RTTI(CstTypePackGeneric)

    explicit CstTypePackGeneric(Position ellipsisPosition);

    Position ellipsisPosition;
};

} // namespace Luau