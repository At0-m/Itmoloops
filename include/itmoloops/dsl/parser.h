#ifndef ITMOLOOPS_DSL_PARSER_H_
#define ITMOLOOPS_DSL_PARSER_H_

#include <string_view>
#include <vector>

#include "itmoloops/dsl/ast.h"
#include "itmoloops/dsl/diagnostic.h"
#include "itmoloops/dsl/lexer.h"

namespace itmoloops::dsl{
  class Parser{
  public:
    static Module Parse(std::string_view text, Diagnostics* diags);
  private:
    Parser(const std::vector<Token>& toks, Diagnostics* diags);

    const Token& Peek(std::size_t k = 0) const;
    bool Match(TokenKind kind);
    const Token& Consume(TokenKind kind, const char* what);
    void SkipNewLines();

    bool IsKeyword(const Token& t, std::string_view kw) const;

    Module ParseModule();
    void ParseBpm(Module* m);
    void ParseInstrument(Module* m);
    void ParsePattern(Module* m);

    bool ParseKeyValue(std::unordered_map<std::string, std::string>* out);
    EffectSpec ParseEffectLine();

    std::unique_ptr<Stmt> ParseStmt();
    std::unique_ptr<Stmt> ParseNoteStmt(const Token& start_tok);
    std::unique_ptr<Stmt> ParseCallStmt(const Token& start_tok);

    const std::vector<Token>& toks_;
    Diagnostics* diags_;
    std::size_t pos_ = 0;
  };
} // namespace itmoloops::dsl
#endif  // ITMOLOOPS_DSL_PARSER_H_