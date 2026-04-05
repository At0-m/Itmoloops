#include "itmoloops/dsl/parser.h"

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <utility>
#include <memory>
 
#include "itmoloops/util/parse_utils.h"
#include "itmoloops/dsl/lexer.h"
#include "itmoloops/dsl/diagnostic.h"
#include "itmoloops/dsl/ast.h"


using itmoloops::util::ParseDoubleStrict;
using itmoloops::util::ParseIntStrict;
using itmoloops::util::ParseI64Strict;

namespace itmoloops::dsl {
namespace {

  bool IEquals(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
      char ca = std::tolower(a[i]);
      char cb = std::tolower(b[i]);
      if (ca != cb) return false;
    }
    return true;
  }

}  // namespace

  Parser::Parser(const std::vector<Token>& toks, Diagnostics* diags)
      : toks_(toks), diags_(diags) {}

  const Token& Parser::Peek(std::size_t k) const {
    std::size_t idx = pos_ + k;
    if (idx >= toks_.size()) return toks_.back();
    return toks_[idx];
  }

  bool Parser::Match(TokenKind kind) {
    if (Peek().kind == kind) { 
      ++pos_; 
      return true; 
    }
    return false;
  }

  const Token& Parser::Consume(TokenKind kind, const char* what) {
    if (Peek().kind != kind) {
      diags_->Error(Peek().range.begin, std::string("expected ") + what);
      return Peek();
    }
    return toks_[pos_++];
  }

  void Parser::SkipNewLines() {
    while (Peek().kind == TokenKind::kNewLine) ++pos_;
  }

  bool Parser::IsKeyword(const Token& t, std::string_view kw) const {
    return t.kind == TokenKind::kIdentifier && IEquals(t.lexem, kw);
  }

  Module Parser::ParseModule() {
    Module m;
    while (Peek().kind != TokenKind::kEof) {
      SkipNewLines();
      const Token& t = Peek();
      if (t.kind == TokenKind::kEof) break;
      if (IsKeyword(t, "bpm"))         { ParseBpm(&m);        continue; }
      if (IsKeyword(t, "instrument"))  { ParseInstrument(&m); continue; }
      if (IsKeyword(t, "pattern"))     { ParsePattern(&m);    continue; }

      diags_->Error(t.range.begin, "unknown top-level directive: '" + t.lexem + "'");
      while (Peek().kind != TokenKind::kNewLine && Peek().kind != TokenKind::kEof) ++pos_;
    }
    return m;
  }

  void Parser::ParseBpm(Module* m) {
    Consume(TokenKind::kIdentifier, "'bpm'");
    SkipNewLines();
    const Token& v = Consume(TokenKind::kNumber, "number");

    double bpm = 0.0;
    if (!ParseDoubleStrict(v.lexem, &bpm)) {
      diags_->Error(v.range.begin, "invalid bpm value");
    } else {
      m->bpm = bpm;
      m->bpm_set = true;
    }

    while (Peek().kind != TokenKind::kNewLine && Peek().kind != TokenKind::kEof) ++pos_;
  }

  bool Parser::ParseKeyValue(std::unordered_map<std::string, std::string>* out) {
    if (Peek().kind != TokenKind::kIdentifier) return false;
    std::string key = Peek().lexem;
    ++pos_;

    Consume(TokenKind::kEqual, "'='");
    if (Peek().kind != TokenKind::kIdentifier && Peek().kind != TokenKind::kNumber) {
      diags_->Error(Peek().range.begin, "expected value after '='");
      return false;
    }

    std::string value = Peek().lexem;
    ++pos_;

    if (Peek().kind == TokenKind::kComma) {
      ++pos_;
      if (Peek().kind != TokenKind::kNumber) {
        diags_->Error(Peek().range.begin, "expected number after comma");
      } else {
        value += ",";
        value += Peek().lexem;
        ++pos_;
      }
    }

    (*out)[key] = value;
    return true;
  }

  EffectSpec Parser::ParseEffectLine() {
    EffectSpec e;
    Consume(TokenKind::kIdentifier, "'effect'");
    const Token& type = Consume(TokenKind::kIdentifier, "effect type");
    e.type = type.lexem;
    e.where.begin = type.range.begin;

    while (Peek().kind != TokenKind::kNewLine && Peek().kind != TokenKind::kEof) {
      if (!ParseKeyValue(&e.params)) {
        diags_->Error(Peek().range.begin, "expected key=value in effect");
        while (Peek().kind != TokenKind::kNewLine && Peek().kind != TokenKind::kEof) ++pos_;
        break;
      }
    }

    e.where.end = Peek().range.end;
    return e;
  }

  void Parser::ParseInstrument(Module* m) {
    Consume(TokenKind::kIdentifier, "'instrument'");

    InstrumentSpec inst;
    const Token& name = Consume(TokenKind::kIdentifier, "instrument name");
    const Token& type = Consume(TokenKind::kIdentifier, "instrument type");
    inst.name = name.lexem;
    inst.type = type.lexem;
    inst.where.begin = name.range.begin;

    while (true) {
      SkipNewLines();
      if (Peek().kind == TokenKind::kEof) {
        diags_->Error(Peek().range.begin, "unexpected EOF in instrument");
        break;
      }
      if (IsKeyword(Peek(), "end")) {
        ++pos_;
        inst.where.end = Peek().range.end;
        break;
      }
      if (IsKeyword(Peek(), "effect")) {
        inst.effects.push_back(ParseEffectLine());
        if (Peek().kind == TokenKind::kNewLine) ++pos_;
        continue;
      }
      if (!ParseKeyValue(&inst.params)) {
        diags_->Error(Peek().range.begin, "expected 'effect' or key=value in instrument");
        while (Peek().kind != TokenKind::kNewLine && Peek().kind != TokenKind::kEof) ++pos_;
      }
      if (Peek().kind == TokenKind::kNewLine) ++pos_;
    }

    m->instruments.push_back(std::move(inst));
  }

  std::unique_ptr<Stmt> Parser::ParseCallStmt(const Token& start_tok) {
    auto call = std::make_unique<CallStmt>();
    if (!ParseI64Strict(start_tok.lexem, &call->start_units)) {
      diags_->Error(start_tok.range.begin, "invalid call start time");
      call->start_units = 0;
    }

    Consume(TokenKind::kAt, "'@'");
    const Token& callee = Consume(TokenKind::kIdentifier, "pattern name after '@'");
    call->callee = callee.lexem;

    call->where.begin = start_tok.range.begin;
    call->where.end = callee.range.end;
    return call;
  }

  std::unique_ptr<Stmt> Parser::ParseNoteStmt(const Token& start_tok) {
    auto note = std::make_unique<NoteStmt>();
    if (!ParseI64Strict(start_tok.lexem, &note->start_units)) {
      diags_->Error(start_tok.range.begin, "invalid note start time");
      note->start_units = 0;
    }

    const Token& inst = Consume(TokenKind::kIdentifier, "instrument name");
    note->instrument = inst.lexem;
    const Token& pitch = Consume(TokenKind::kIdentifier, "pitch");
    note->pitch = pitch.lexem;

    const Token& dur = Consume(TokenKind::kNumber, "duration");
    const Token& vel = Consume(TokenKind::kNumber, "velocity");

    if (!ParseI64Strict(dur.lexem, &note->duration_units)) {
      diags_->Error(dur.range.begin, "invalid duration");
      note->duration_units = 0;
    }
    if (!ParseIntStrict(vel.lexem, &note->velocity)) {
      diags_->Error(vel.range.begin, "invalid velocity");
      note->velocity = 0;
    }

    note->where.begin = start_tok.range.begin;
    note->where.end = vel.range.end;
    return note;
  }

  std::unique_ptr<Stmt> Parser::ParseStmt() {
    Token start_tok = Peek();
    ++pos_;
    SkipNewLines();
    if (Peek().kind == TokenKind::kAt) return ParseCallStmt(start_tok);
    return ParseNoteStmt(start_tok);
  }

  void Parser::ParsePattern(Module* m) {
    Consume(TokenKind::kIdentifier, "'pattern'");

    Pattern p;
    const Token& name = Consume(TokenKind::kIdentifier, "pattern name");
    p.name = name.lexem;
    p.where.begin = name.range.begin;

    SkipNewLines();

    int res = 0;
    if (IsKeyword(Peek(), "resolution")) {
      ++pos_;
      const Token& v = Consume(TokenKind::kNumber, "resolution number");
      if (!ParseIntStrict(v.lexem, &res)) {
        diags_->Error(v.range.begin, "invalid resolution");
        res = 0;
      }
    } else if (Peek().kind == TokenKind::kNumber) {
      if (!ParseIntStrict(Peek().lexem, &res)) {
        diags_->Error(Peek().range.begin, "invalid resolution");
        res = 0;
      }
      ++pos_;
    } else {
      diags_->Error(Peek().range.begin, "expected 'resolution N' or integer after pattern name");
    }

    if (res <= 0) {
      diags_->Error(name.range.begin, "pattern resolution must be positive");
      res = 1;
    }
    p.resolution = res;

    while (true) {
      SkipNewLines();
      if (Peek().kind == TokenKind::kEof) {
        diags_->Error(Peek().range.begin, "unexpected EOF in pattern");
        break;
      }
      if (IsKeyword(Peek(), "end")) {
        ++pos_;
        p.where.end = Peek().range.end;
        break;
      }
      if (Peek().kind != TokenKind::kNumber) {
        diags_->Error(Peek().range.begin, "expected statement starting with start time");
        while (Peek().kind != TokenKind::kNewLine && Peek().kind != TokenKind::kEof) ++pos_;
        if (Peek().kind == TokenKind::kNewLine) ++pos_;
        continue;
      }
      p.body.push_back(ParseStmt());
      if (Peek().kind == TokenKind::kNewLine) ++pos_;
    }

    m->patterns.push_back(std::move(p));
  }

  Module Parser::Parse(std::string_view text, Diagnostics* diags) {
    std::vector<Token> toks = Lexer::Tokenize(text);
    Parser par(toks, diags);
    return par.ParseModule();
  }

}  // namespace itmoloops::dsl