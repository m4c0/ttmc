#pragma leco tool
#include <stdio.h>

import hai;
import hashley;
import hay;
import jojo;
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

class scanner {
  hai::cstr m_data;
  unsigned m_rpos = 0;
  unsigned m_wpos = 0;

public:
  explicit scanner(hai::cstr data) : m_data { traits::move(data) } {}

  explicit operator bool() const { return m_rpos < m_data.size(); }

  char getc() {
    // Assuming text files, so all bytes below 32 (except TAB, CR, LF)
    // are ours to take.
    if (m_rpos == m_data.size()) return 0;
    return m_data.data()[m_rpos++];
  }

  void skip(unsigned n = 1) {
    for (auto i = 0; i < n; i++) {
      if (m_wpos >= m_rpos) die("scanner underflow");
      if (m_wpos >= m_data.size()) die("scanner overflow");
      m_data.data()[m_wpos++] = m_data.data()[m_rpos - n + i];
    }
  }
  void spc() {
    if (m_wpos >= m_data.size()) die("scanner overflow");
    m_data.data()[m_wpos++] = '!';
  }

  void dump() {
    m_data.data()[m_wpos++] = 0;
    putln(m_data);
  }
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

static void run(buffer & in, scanner & f) {
  buffer tmp {};
  run(in, tmp);
  putln("run that");
  // TODO: unget into f
}

static void parser(scanner & f);

static void parse_at(scanner & f) {
  switch (f.getc()) {
    case 0: break;
    default: f.skip(); break;
  }
}

static void parse_dpound(scanner & f) {
  switch (f.getc()) {
    case '<': {
      parser(f);
      //if (f) run(b, buf);
      break;
    }
    default: 
      f.skip(3);
      break;
  }
}

static void parse_lt(scanner & f) {
  while (f) {
    switch (f.getc()) {
      case 0:
        break;
      case '@':
        parse_at(f);
        break;
      case '>':
        return;
      case '<': 
        f.skip();
        parse_lt(f);
        if (f) f.skip();
        break;
      default:
        f.skip();
        break;
    }
  }
}

static void parse_pound(scanner & f) {
  switch (f.getc()) {
    case '#':
      parse_dpound(f);
      break;
    case '<': {
      parser(f);
      //if (f) run(b, f);
      break;
    }
    default: 
      f.skip(2);
      break;
  }
};

static void parser(scanner & f) {
  while (f) {
    switch (f.getc()) {
      case 0:  break;
      case '@': parse_at(f); break;
      case '#': parse_pound(f); break;
      case '<': parse_lt(f); break;
      case ';': f.spc(); break;
      case '>': f.spc(); return;
      default:  f.skip(); break;
    }
  }
}

int main() {
  scanner s { jojo::read_cstr("example.ttm") };
  parser(s);
  s.dump();
}
