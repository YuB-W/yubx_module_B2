// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#pragma once

#include "Luau/Ast.h"
#include "Luau/Lexer.h"
#include "Luau/ParseOptions.h"
#include "Luau/ParseResult.h"
#include "Luau/StringUtils.h"
#include "Luau/DenseHash.h"
#include "Luau/Common.h"
#include "Luau/Cst.h"

#include <initializer_list>
#include <optional>
#include <tuple>

namespace Luau
{

template<typename T>
class TempVector
{
public:
    explicit TempVector(std::vector<T>& storage);

    ~TempVector();

    const T& operator[](std::size_t index) const;

    const T& front() const;

    const T& back() const;

    bool empty() const;

    std::size_t size() const;

    void push_back(const T& item);

    typename std::vector<T>::const_iterator begin() const
    {
        return storage.begin() + offset;
    }
    typename std::vector<T>::const_iterator end() const
    {
        return storage.begin() + offset + size_;
    }

private:
    std::vector<T>& storage;
    size_t offset;
    size_t size_;
};

class Parser
{
public:
    static ParseResult parse(
        const char* buffer,
        std::size_t bufferSize,
        AstNameTable& names,
        Allocator& allocator,
        ParseOptions options = ParseOptions()
    );

    static ParseExprResult parseExpr(
        const char* buffer,
        std::size_t bufferSize,
        AstNameTable& names,
        Allocator& allocator,
        ParseOptions options = ParseOptions()
    );

private:
    struct Name;
    struct Binding;

    Parser(const char* buffer, std::size_t bufferSize, AstNameTable& names, Allocator& allocator, const ParseOptions& options);

    bool blockFollow(const Lexeme& l);

    AstStatBlock* parseChunk();

    // chunk ::= {stat [`;']} [laststat [`;']]
    // block ::= chunk
    AstStatBlock* parseBlock();

    AstStatBlock* parseBlockNoScope();

    // stat ::=
    // varlist `=' explist |
    // functioncall |
    // do block end |
    // while exp do block end |
    // repeat block until exp |
    // if exp then block {elseif exp then block} [else block] end |
    // for Name `=' exp `,' exp [`,' exp] do block end |
    // for namelist in explist do block end |
    // [attributes] function funcname funcbody |
    // [attributes] local function Name funcbody |
    // local namelist [`=' explist]
    // laststat ::= return [explist] | break
    AstStat* parseStat();

    // if exp then block {elseif exp then block} [else block] end
    AstStat* parseIf();

    // while exp do block end
    AstStat* parseWhile();

    // repeat block until exp
    AstStat* parseRepeat();

    // do block end
    AstStat* parseDo();

    // break
    AstStat* parseBreak();

    // continue
    AstStat* parseContinue(const Location& start);

    // for Name `=' exp `,' exp [`,' exp] do block end |
    // for namelist in explist do block end |
    AstStat* parseFor();

    // funcname ::= Name {`.' Name} [`:' Name]
    AstExpr* parseFunctionName(bool& hasself, AstName& debugname);

    // function funcname funcbody
    LUAU_FORCEINLINE AstStat* parseFunctionStat(const AstArray<AstAttr*>& attributes = {nullptr, 0});

    std::pair<bool, AstAttr::Type> validateAttribute(const char* attributeName, const TempVector<AstAttr*>& attributes);

    // attribute ::= '@' NAME
    void parseAttribute(TempVector<AstAttr*>& attribute);

    // attributes ::= {attribute}
    AstArray<AstAttr*> parseAttributes();

    // attributes local function Name funcbody
    // attributes function funcname funcbody
    // attributes `declare function' Name`(' [parlist] `)' [`:` Type]
    // declare Name '{' Name ':' attributes `(' [parlist] `)' [`:` Type] '}'
    AstStat* parseAttributeStat();

    // local function Name funcbody |
    // local namelist [`=' explist]
    AstStat* parseLocal(const AstArray<AstAttr*>& attributes);

    // return [explist]
    AstStat* parseReturn();

    // type Name `=' Type
    AstStat* parseTypeAlias(const Location& start, bool exported, Position typeKeywordPosition);

    // type function Name ... end
    AstStat* parseTypeFunction(const Location& start, bool exported, Position typeKeywordPosition);

    AstDeclaredExternTypeProperty parseDeclaredExternTypeMethod(const AstArray<AstAttr*>& attributes);
    AstDeclaredExternTypeProperty parseDeclaredExternTypeMethod_DEPRECATED();


    // `declare global' Name: Type |
    // `declare function' Name`(' [parlist] `)' [`:` Type]
    AstStat* parseDeclaration(const Location& start, const AstArray<AstAttr*>& attributes);

    // varlist `=' explist
    AstStat* parseAssignment(AstExpr* initial);

    // var [`+=' | `-=' | `*=' | `/=' | `%=' | `^=' | `..='] exp
    AstStat* parseCompoundAssignment(AstExpr* initial, AstExprBinary::Op op);

    std::pair<AstLocal*, AstArray<AstLocal*>> prepareFunctionArguments(const Location& start, bool hasself, const TempVector<Binding>& args);

    // funcbodyhead ::= `(' [namelist [`,' `...'] | `...'] `)' [`:` Type]
    // funcbody ::= funcbodyhead block end
    std::pair<AstExprFunction*, AstLocal*> parseFunctionBody(
        bool hasself,
        const Lexeme& matchFunction,
        const AstName& debugname,
        const Name* localName,
        const AstArray<AstAttr*>& attributes
    );
    // Clip with FFlagLuauStoreReturnTypesAsPackOnAst
    std::pair<AstExprFunction*, AstLocal*> parseFunctionBody_DEPRECATED(
        bool hasself,
        const Lexeme& matchFunction,
        const AstName& debugname,
        const Name* localName,
        const AstArray<AstAttr*>& attributes
    );

    // explist ::= {exp `,'} exp
    void parseExprList(TempVector<AstExpr*>& result, TempVector<Position>* commaPositions = nullptr);

    // binding ::= Name [`:` Type]
    Binding parseBinding();
    AstArray<Position> extractAnnotationColonPositions(const TempVector<Binding>& bindings);

    // bindinglist ::= (binding | `...') {`,' bindinglist}
    // Returns the location of the vararg ..., or std::nullopt if the function is not vararg.
    std::tuple<bool, Location, AstTypePack*> parseBindingList(
        TempVector<Binding>& result,
        bool allowDot3 = false,
        AstArray<Position>* commaPositions = nullptr,
        Position* initialCommaPosition = nullptr,
        Position* varargAnnotationColonPosition = nullptr
    );

    AstType* parseOptionalType();

    // TypeList ::= Type [`,' TypeList]
    // ReturnType ::= Type | `(' TypeList `)'
    // TableProp ::= Name `:' Type
    // TableIndexer ::= `[' Type `]' `:' Type
    // PropList ::= (TableProp | TableIndexer) [`,' PropList]
    // Type
    //      ::= Name
    //      |   `nil`
    //      |   `{' [PropList] `}'
    //      |   `(' [TypeList] `)' `->` ReturnType

    // Returns the variadic annotation, if it exists.
    AstTypePack* parseTypeList(
        TempVector<AstType*>& result,
        TempVector<std::optional<AstArgumentName>>& resultNames,
        TempVector<Position>* commaPositions = nullptr,
        TempVector<std::optional<Position>>* nameColonPositions = nullptr
    );

    AstTypePack* parseOptionalReturnType(Position* returnSpecifierPosition = nullptr);
    AstTypePack* parseReturnType();

    // Clip with FFlagLuauStoreReturnTypesAsPackOnAst
    std::optional<AstTypeList> parseOptionalReturnType_DEPRECATED(Position* returnSpecifierPosition = nullptr);
    std::pair<Location, AstTypeList> parseReturnType_DEPRECATED();

    struct TableIndexerResult
    {
        AstTableIndexer* node;
        Position indexerOpenPosition;
        Position indexerClosePosition;
        Position colonPosition;
    };

    TableIndexerResult parseTableIndexer(AstTableAccess access, std::optional<Location> accessLocation, Lexeme begin);

    AstTypeOrPack parseFunctionType(bool allowPack, const AstArray<AstAttr*>& attributes);
    AstType* parseFunctionTypeTail(
        const Lexeme& begin,
        const AstArray<AstAttr*>& attributes,
        AstArray<AstGenericType*> generics,
        AstArray<AstGenericTypePack*> genericPacks,
        AstArray<AstType*> params,
        AstArray<std::optional<AstArgumentName>> paramNames,
        AstTypePack* varargAnnotation
    );

    AstType* parseTableType(bool inDeclarationContext = false);
    AstTypeOrPack parseSimpleType(bool allowPack, bool inDeclarationContext = false);

    AstTypeOrPack parseSimpleTypeOrPack();
    AstType* parseType(bool inDeclarationContext = false);

    AstTypePack* parseTypePack();
    AstTypePack* parseVariadicArgumentTypePack();

    AstType* parseTypeSuffix(AstType* type, const Location& begin);

    static std::optional<AstExprUnary::Op> parseUnaryOp(const Lexeme& l);
    static std::optional<AstExprBinary::Op> parseBinaryOp(const Lexeme& l);
    static std::optional<AstExprBinary::Op> parseCompoundOp(const Lexeme& l);

    struct BinaryOpPriority
    {
        unsigned char left, right;
    };

    std::optional<AstExprUnary::Op> checkUnaryConfusables();
    std::optional<AstExprBinary::Op> checkBinaryConfusables(const BinaryOpPriority binaryPriority[], unsigned int limit);

    // subexpr -> (asexp | unop subexpr) { binop subexpr }
    // where `binop' is any binary operator with a priority higher than `limit'
    AstExpr* parseExpr(unsigned int limit = 0);

    // NAME
    AstExpr* parseNameExpr(const char* context = nullptr);

    // prefixexp -> NAME | '(' expr ')'
    AstExpr* parsePrefixExpr();

    // primaryexp -> prefixexp { `.' NAME | `[' exp `]' | `:' NAME funcargs | funcargs }
    AstExpr* parsePrimaryExpr(bool asStatement);

    // asexp -> simpleexp [`::' Type]
    AstExpr* parseAssertionExpr();

    // simpleexp -> NUMBER | STRING | NIL | true | false | ... | constructor | [attributes] FUNCTION body | primaryexp
    AstExpr* parseSimpleExpr();

    // args ::=  `(' [explist] `)' | tableconstructor | String
    AstExpr* parseFunctionArgs(AstExpr* func, bool self);

    std::optional<CstExprTable::Separator> tableSeparator();

    // tableconstructor ::= `{' [fieldlist] `}'
    // fieldlist ::= field {fieldsep field} [fieldsep]
    // field ::= `[' exp `]' `=' exp | Name `=' exp | exp
    // fieldsep ::= `,' | `;'
    AstExpr* parseTableConstructor();

    // TODO: Add grammar rules here?
    AstExpr* parseIfElseExpr();

    // stringinterp ::= <INTERP_BEGIN> exp {<INTERP_MID> exp} <INTERP_END>
    AstExpr* parseInterpString();

    // Name
    std::optional<Name> parseNameOpt(const char* context = nullptr);
    Name parseName(const char* context = nullptr);
    Name parseIndexName(const char* context, const Position& previous);

    // `<' namelist `>'
    std::pair<AstArray<AstGenericType*>, AstArray<AstGenericTypePack*>> parseGenericTypeList(
        bool withDefaultValues,
        Position* openPosition = nullptr,
        AstArray<Position>* commaPositions = nullptr,
        Position* closePosition = nullptr
    );

    // `<' Type[, ...] `>'
    AstArray<AstTypeOrPack> parseTypeParams(
        Position* openingPosition = nullptr,
        TempVector<Position>* commaPositions = nullptr,
        Position* closingPosition = nullptr
    );

    std::optional<AstArray<char>> parseCharArray(AstArray<char>* originalString = nullptr);
    AstExpr* parseString();
    AstExpr* parseNumber();

    AstLocal* pushLocal(const Binding& binding);

    unsigned int saveLocals();

    void restoreLocals(unsigned int offset);

    /// Returns string quote style and block depth
    std::pair<CstExprConstantString::QuoteStyle, unsigned int> extractStringDetails();

    // check that parser is at lexeme/symbol, move to next lexeme/symbol on success, report failure and continue on failure
    bool expectAndConsume(char value, const char* context = nullptr);
    bool expectAndConsume(Lexeme::Type type, const char* context = nullptr);
    void expectAndConsumeFail(Lexeme::Type type, const char* context);

    struct MatchLexeme
    {
        MatchLexeme(const Lexeme& l)
            : type(l.type)
            , position(l.location.begin)
        {
        }

        Lexeme::Type type;
        Position position;
    };

    bool expectMatchAndConsume(char value, const MatchLexeme& begin, bool searchForMissing = false);
    void expectMatchAndConsumeFail(Lexeme::Type type, const MatchLexeme& begin, const char* extra = nullptr);
    bool expectMatchAndConsumeRecover(char value, const MatchLexeme& begin, bool searchForMissing);

    bool expectMatchEndAndConsume(Lexeme::Type type, const MatchLexeme& begin);
    void expectMatchEndAndConsumeFail(Lexeme::Type type, const MatchLexeme& begin);

    template<typename T>
    AstArray<T> copy(const T* data, std::size_t size);

    template<typename T>
    AstArray<T> copy(const TempVector<T>& data);

    template<typename T>
    AstArray<T> copy(std::initializer_list<T> data);

    AstArray<char> copy(const std::string& data);

    void incrementRecursionCounter(const char* context);

    void report(const Location& location, const char* format, va_list args);
    void report(const Location& location, const char* format, ...) LUAU_PRINTF_ATTR(3, 4);

    void reportNameError(const char* context);

    AstStatError* reportStatError(
        const Location& location,
        const AstArray<AstExpr*>& expressions,
        const AstArray<AstStat*>& statements,
        const char* format,
        ...
    ) LUAU_PRINTF_ATTR(5, 6);
    AstExprError* reportExprError(const Location& location, const AstArray<AstExpr*>& expressions, const char* format, ...) LUAU_PRINTF_ATTR(4, 5);
    AstTypeError* reportTypeError(const Location& location, const AstArray<AstType*>& types, const char* format, ...) LUAU_PRINTF_ATTR(4, 5);
    // `parseErrorLocation` is associated with the parser error
    // `astErrorLocation` is associated with the AstTypeError created
    // It can be useful to have different error locations so that the parse error can include the next lexeme, while the AstTypeError can precisely
    // define the location (possibly of zero size) where a type annotation is expected.
    AstTypeError* reportMissingTypeError(const Location& parseErrorLocation, const Location& astErrorLocation, const char* format, ...)
        LUAU_PRINTF_ATTR(4, 5);

    AstExpr* reportFunctionArgsError(AstExpr* func, bool self);
    void reportAmbiguousCallError();

    void nextLexeme();

    struct Function
    {
        bool vararg;
        unsigned int loopDepth;

        Function()
            : vararg(false)
            , loopDepth(0)
        {
        }
    };

    struct Local
    {
        AstLocal* local;
        unsigned int offset;

        Local()
            : local(nullptr)
            , offset(0)
        {
        }
    };

    struct Name
    {
        AstName name;
        Location location;

        Name(const AstName& name, const Location& location)
            : name(name)
            , location(location)
        {
        }
    };

    struct Binding
    {
        Name name;
        AstType* annotation;
        Position colonPosition;

        explicit Binding(const Name& name, AstType* annotation = nullptr, Position colonPosition = {0,0})
            : name(name)
            , annotation(annotation)
            , colonPosition(colonPosition)
        {
        }
    };

    ParseOptions options;

    Lexer lexer;
    Allocator& allocator;

    std::vector<Comment> commentLocations;
    std::vector<HotComment> hotcomments;

    bool hotcommentHeader = true;

    unsigned int recursionCounter;

    AstName nameSelf;
    AstName nameNumber;
    AstName nameError;
    AstName nameNil;

    MatchLexeme endMismatchSuspect;

    std::vector<Function> functionStack;
    size_t typeFunctionDepth = 0;

    DenseHashMap<AstName, AstLocal*> localMap;
    std::vector<AstLocal*> localStack;

    std::vector<ParseError> parseErrors;

    std::vector<unsigned int> matchRecoveryStopOnToken;

    std::vector<AstAttr*> scratchAttr;
    std::vector<AstStat*> scratchStat;
    std::vector<AstArray<char>> scratchString;
    std::vector<AstArray<char>> scratchString2;
    std::vector<AstExpr*> scratchExpr;
    std::vector<AstExpr*> scratchExprAux;
    std::vector<AstName> scratchName;
    std::vector<AstName> scratchPackName;
    std::vector<Binding> scratchBinding;
    std::vector<AstLocal*> scratchLocal;
    std::vector<AstTableProp> scratchTableTypeProps;
    std::vector<CstTypeTable::Item> scratchCstTableTypeProps;
    std::vector<AstType*> scratchType;
    std::vector<AstTypeOrPack> scratchTypeOrPack;
    std::vector<AstDeclaredExternTypeProperty> scratchDeclaredClassProps;
    std::vector<AstExprTable::Item> scratchItem;
    std::vector<CstExprTable::Item> scratchCstItem;
    std::vector<AstArgumentName> scratchArgName;
    std::vector<AstGenericType*> scratchGenericTypes;
    std::vector<AstGenericTypePack*> scratchGenericTypePacks;
    std::vector<std::optional<AstArgumentName>> scratchOptArgName;
    std::vector<Position> scratchPosition;
    std::vector<std::optional<Position>> scratchOptPosition;
    std::string scratchData;

    CstNodeMap cstNodeMap;
};

} // namespace Luau
