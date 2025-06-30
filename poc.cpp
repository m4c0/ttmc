#pragma leco tool

import hai;
import hashley;
import jojo;
import jute;
import print;

class scanner {
  hai::cstr m_data {};
  unsigned m_rpos = 0;
  unsigned m_wpos = 0;

public:
  constexpr scanner() = default;

  explicit scanner(hai::cstr data) : m_data { traits::move(data) } {}

  explicit operator bool() const { return m_rpos < m_data.size(); }

  auto view() const { return jute::view::unsafe(m_data.data()); }
  auto r_view() const { return jute::view::unsafe(m_data.data() + m_rpos); }
  auto w_ptr() const { return m_data.data() + m_wpos; }

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
  void spc(char c = 0) {
    if (m_wpos >= m_data.size()) die("scanner overflow");
    m_data.data()[m_wpos++] = c;
  }

  jute::view arg_after(jute::view arg) {
    if (arg.begin() == 0) return arg;

    auto a = arg.end() + 1;
    if (a >= m_data.end()) return {};
    if (a >= m_data.begin() + m_wpos) return {};

    return jute::view::unsafe(a);
  }
};

static hashley::fin<scanner> g_mem { 127 }; 

static void ss(scanner & f, jute::view key) {
  auto & val = g_mem[key];

  while (val) {
    auto arg = f.arg_after(key);
    char i = 1;
    while (arg.begin()) {
      auto v = val.r_view().subview(arg.size()).before;
      if (v == arg) {
        for (auto i = 0; i < v.size(); i++) val.getc();
        val.spc(i);
        i = -1;
        break;
      }

      arg = f.arg_after(arg);
      i++;
      if (i == 7) die("too many arguments to ss of ", key);
    }

    if (i != -1) {
      val.getc();
      val.skip();
    }
  }

  val.spc();
  val = scanner { val.view().cstr() };
}

static void run(scanner & f, const char * mark) {
  auto fn = jute::view::unsafe(mark);
  if (fn == "ds") {
    auto key = f.arg_after(fn);
    auto val = f.arg_after(key);
    g_mem[key] = scanner { val.cstr() };
  } else if (fn == "ss") {
    auto key = f.arg_after(fn);
    ss(f, key);
  } else {
    putln("fn: ", g_mem[fn].view());

    auto arg = f.arg_after(fn);
    while (arg.begin()) {
      putln("- ", arg);
      arg = f.arg_after(arg);
    }
    putln();
  }
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
      auto mark = f.w_ptr();
      parser(f);
      if (f) run(f, mark);
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
      auto mark = f.w_ptr();
      parser(f);
      if (f) run(f, mark);
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

int main() try {
  scanner s { jojo::read_cstr("example.ttm") };
  parser(s);
} catch (...) {
  return 1;
}
