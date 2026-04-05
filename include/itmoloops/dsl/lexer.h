#ifndef ITMOLOOPS_DSL_LEXER_H_
#define ITMOLOOPS_DSL_LEXER_H_

#include <string_view>
#include <string>
#include <vector>

#include "itmoloops/dsl/source.h"

namespace itmoloops::dsl{
  enum class TokenKind{
    kEof, kNewLine, kIdentifier, 
    kNumber, kAt, kEqual, kComma
  };

  struct Token
  {
    TokenKind kind = TokenKind::kEof;
    std::string lexem;
    Range range;
  };

  class Lexer{
  public:
    static std::vector<Token> Tokenize(std::string_view text);
  private:
    explicit Lexer(std::string_view text);

    char Peek() const;
    char PeekN(std::size_t n) const;
    char Get();
    bool Eof() const;

    void Emit(TokenKind kind, std::string_view lex, std::size_t start_off,
              std::size_t end_off, std::vector<Token>* out);
    
    bool IsCommentStart() const;
    void SkipSpaces();
    void SkipComment();
    void ReadNewline(std::vector<Token>* out);
    void ReadNumber(std::vector<Token>* out);
    void ReadAtom(std::vector<Token>* out);

    const char* data_;
    std::size_t size_;
    std::size_t off_ = 0;
    std::size_t line_= 1;
    std::size_t col_ = 1;
    bool at_token_start_ = true;
  };

} // namespace itmoloops::dsl

#endif // ITMOLOOPS_DSL_LEXER_H_