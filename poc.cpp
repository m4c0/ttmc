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
  void dump() {
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

static void run(buffer & in, buffer & out) {
  put("run: ");
  in.dump();
  putln();
}

static void run(buffer & in, file & f) {
  buffer tmp {};
  run(in, tmp);
  // TODO: unget into f
}

static void parser(buffer & buf, file & f);

static void parse_at(buffer & buf, file & f) {
  switch (char c = f.getc()) {
    case -1: break;
    default: buf.push_char(c); break;
  }
}

static void parse_dpound(buffer & buf, file & f) {
  switch (char c = f.getc()) {
    case '<': {
      buffer b {};
      parser(b, f);
      if (f) run(b, buf);
      break;
    }
    default: 
      buf.push_char('#');
      buf.push_char('#');
      buf.push_char(c);
      break;
  }
}

static void parse_lt(buffer & buf, file & f) {
  while (f) {
    switch (char c = f.getc()) {
      case -1:
        break;
      case '@':
        parse_at(buf, f);
        break;
      case '>':
        return;
      case '<': 
        buf.push_char(c);
        parse_lt(buf, f);
        if (f) buf.push_char('>');
        break;
      default:
        buf.push_char(c);
        break;
    }
  }
}

static void parse_pound(buffer & buf, file & f) {
  switch (char c = f.getc()) {
    case '#':
      parse_dpound(buf, f);
      break;
    case '<': {
      buffer b {};
      parser(b, f);
      if (f) run(b, f);
      break;
    }
    default: 
      buf.push_char('#');
      buf.push_char(c);
      break;
  }
};

static void parser(buffer & buf, file & f) {
  while (f) {
    switch (char c = f.getc()) {
      case -1:  break;
      case '@': parse_at(buf, f); break;
      case '#': parse_pound(buf, f); break;
      case '<': parse_lt(buf, f); break;
      case ';': buf.push_spc('^'); break;
      case '>': return;
      default:  buf.push_char(c); break;
    }
  }
}

int main() {
  buffer buf {};
  file f {};
  parser(buf, f);
}
