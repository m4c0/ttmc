#pragma leco tool
#include <stdio.h>

import hay;
import print;

struct glyph {
  char value;
  bool special;
};

class buffer {
  static constexpr const auto max_glyphs = 1024;

  glyph m_data[max_glyphs] {};
  int m_wpos = 0;

  void push(glyph g) {
    if (m_wpos == max_glyphs) die("buffer overflow while processing pending strings");
    m_data[m_wpos++] = g;
  }

public:
  ~buffer() {
    for (auto i = 0; i < m_wpos; i++) put(m_data[i].value);
  }

  void push_char(char c) { push({ c }); }
  void push_spc(char c) { push({ c, true }); }
};

class file {
  hay<FILE *, fopen, fclose> m_f { "example.ttm", "rb" };

public:
  int getc() {
    auto res = fgetc(m_f);
    if (res != EOF) return res;
    if (feof(m_f)) return -1;
    die("error reading file: ", ferror(m_f));
  }

  explicit operator bool() { return !feof(m_f); }
};

int main() {
  buffer buf {};
  file f {};

  const auto parse_at = [&] {
    switch (char c = f.getc()) {
      case -1: break;
      default: buf.push_char(c); break;
    }
  };
  const auto parse_dpound = [&] {
    switch (char c = f.getc()) {
      case '<': 
        buf.push_spc('[');
        break;
      default: 
        buf.push_char('#');
        buf.push_char('#');
        buf.push_char(c);
        break;
    }
  };
  const auto parse_pound = [&] {
    switch (char c = f.getc()) {
      case '#': parse_dpound(); break;
      case '<': buf.push_spc('{'); break;
      default: 
        buf.push_char('#');
        buf.push_char(c);
        break;
    }
  };
  const auto parse_lt = [&](auto & parse_lt) {
    while (f) {
      switch (char c = f.getc()) {
        case -1: die("EOF on a open '<'"); break;
        case '@': parse_at(); break;
        case '>': return;
        case '<':
          buf.push_char(c);
          parse_lt(parse_lt);
          buf.push_char('>');
          break;
        default: buf.push_char(c); break;
      }
    }
  };

  while (f) {
    switch (char c = f.getc()) {
      case -1:  break;
      case '@': parse_at(); break;
      case '#': parse_pound(); break;
      case '<': parse_lt(parse_lt); break;
      case ';': buf.push_spc('^'); break;
      default:  buf.push_char(c); break;
    }
  }
}
