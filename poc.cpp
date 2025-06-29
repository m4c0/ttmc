#pragma leco tool
#include <stdio.h>

import hai;
import hashley;
import hay;
import jute;
import print;

struct pair {
  unsigned start;
  unsigned end;
};

class buffer {
  hai::varray<pair> m_specs { 64 };
  hai::varray<char> m_data { 128 };

public:
  auto str(int idx) const {
    if (idx >= m_specs.size()) return jute::view {};

    auto [s, e] = m_specs[idx];
    return jute::view { m_data.begin() + s, e - s };
  }

  auto arity() const { return m_specs.size() - 1; }

  void push_char(char c) {
    m_data.push_back_doubling(c);
  }

  void push_spc() {
    pair p {};
    if (m_specs.size() > 0) p.start = (m_specs.end() - 1)->end;
    p.end = m_data.size();

    m_specs.push_back_doubling(p);
  }
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

static hashley::fin<hai::cstr> g_mem { 127 }; 

static void run(buffer & in, buffer & out) {
  in.push_spc();

  auto fn = in.str(0);
  if (fn == "ds") {
    auto key = in.str(1);
    auto val = in.str(2);
    g_mem[key] = val.cstr();
  } else if (fn == "ss") {
    auto key = in.str(1);
    auto & val = g_mem[key];
    put("ss ", val);
    for (auto i = 1; i < in.arity(); i++) {
      put(" ", in.str(i + 1));
    }
    putln();
  } else {
    put("fn: ", g_mem[in.str(0)]);
    for (auto i = 0; i < in.arity(); i++) {
      put(" !", in.str(i + 1));
    }
    putln();
  }
}

static void run(buffer & in, file & f) {
  buffer tmp {};
  run(in, tmp);
  putln("run that");
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
      case ';': buf.push_spc(); break;
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
