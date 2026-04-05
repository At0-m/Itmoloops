#include "itmoloops/dsl/lexer.h"

#include <cctype>
#include <string_view>
#include <string>
#include <cstddef>
#include <vector>
#include <utility>

namespace itmoloops::dsl{
namespace {
  bool IsSpaceNoNl(char c) {
    return c == ' ' || c == '\t' || c == '\f' || c == '\v';
  }
  bool IsNewLine(char c) {
    return c == '\n' || c == '\r';
  }
} // namespace 

  Lexer::Lexer(std::string_view text) : data_(text.data()), size_(text.size()){}

  char Lexer::Peek() const {
    return off_ < size_ ? data_[off_] : '\0';
  }
  char Lexer::PeekN(std::size_t n) const{
    return (off_ + n < size_) ? data_[off_ + n] : '\0';
  }
  bool Lexer::Eof() const {
    return off_ >= size_;
  }

  char Lexer::Get(){
    if(off_ >= size_) return '\0';
    char c = data_[off_++];
    if(c == '\r'){
      if(Peek() == '\n') ++off_;
      ++line_;
      col_ = 1;
    } else if(c == '\n'){
      ++line_;
      col_ = 1;
    } else {
      ++col_;
    }
    return c;
  }

  void Lexer::Emit(TokenKind kind, std::string_view lex, std::size_t start_off,
              std::size_t end_off, std::vector<Token>* out) {
    Token t;
    t.kind = kind;
    t.lexem.assign(lex.begin(), lex.end());
    t.range.begin = {line_, col_};
    t.range.end = t.range.begin;
    out->push_back(std::move(t));
  }

  bool Lexer::IsCommentStart() const {
    if(Peek() != '#') return false;
    if(off_ == 0) return true;

    char prev = data_[off_ - 1];
    return IsSpaceNoNl(prev) || IsNewLine(prev);
  }
  void Lexer::SkipSpaces(){
    while(!Eof() && IsSpaceNoNl(Peek())) Get();
  }

  void Lexer::SkipComment(){
    while(!Eof() && !IsNewLine(Peek())) Get();
  }

  void Lexer::ReadNewline(std::vector<Token>* out){
    if(Eof()) return;
    if(Peek() == '\r' || Peek() == '\n'){
      Get();
      Token t;
      t.kind = TokenKind::kNewLine;
      t.lexem = "\\n";
      t.range.begin = {line_, col_};
      t.range.end = t.range.begin;
      out->push_back(std::move(t));
      at_token_start_ = true;
    }
  }

  void Lexer::ReadNumber(std::vector<Token>* out){
    std::size_t start = off_;
    bool has_dot = false;
    while(!Eof()){
      char c = Peek();
      if(std::isdigit(c)){
        Get();
        continue;
      }
      if(!has_dot && c == '.'){
        has_dot = true;
        Get();
        continue;
      }
      break;
    }
    std::size_t end = off_;
    Emit(TokenKind::kNumber, std::string_view(data_ + start, end - start), start, end, out);
    at_token_start_ = false;
  }

  void Lexer::ReadAtom(std::vector<Token>* out){
    std::size_t start = off_;
    while(!Eof()){
      char c = Peek();
      if(IsSpaceNoNl(c) || IsNewLine(c) || c == '@' || c == '=' || c == ',') break;
      Get();
    }
    std::size_t end = off_;

    if (end == start && !Eof()) {
        Get();
        end = off_;
    }
    if(end > start){
      Emit(TokenKind::kIdentifier, std::string_view(data_ + start, end - start), start, end, out);
      at_token_start_ = false;
    }
  }

  std::vector<Token> Lexer::Tokenize(std::string_view text) {
    Lexer lex(text);
    std::vector<Token> out;
    while (!lex.Eof()) {
      lex.SkipSpaces();
      if (lex.Eof()) break;

      char c = lex.Peek();
      if (IsNewLine(c)) { lex.ReadNewline(&out); continue; }
      if (lex.IsCommentStart()) {lex.SkipComment(); continue;}
      if (c == '@') { lex.Get(); lex.Emit(TokenKind::kAt, "@", lex.off_ - 1, lex.off_, &out); lex.at_token_start_ = false; continue; }
      if (c == '=') { lex.Get(); lex.Emit(TokenKind::kEqual, "=", lex.off_ - 1, lex.off_, &out); lex.at_token_start_ = false; continue; }
      if (c == ',') { lex.Get(); lex.Emit(TokenKind::kComma, ",", lex.off_ - 1, lex.off_, &out); lex.at_token_start_ = false; continue; }
      if (std::isdigit(c)) { lex.ReadNumber(&out); continue; }
      lex.ReadAtom(&out);
    }
    out.push_back(Token{TokenKind::kEof, "", {{lex.line_, lex.col_}, {lex.line_, lex.col_}}});
    return out;
  }
} // namespace itmoloops::dsl