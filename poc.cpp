#pragma leco tool

import hai;
import hashley;
import jojo;
import jute;
import print;

static constexpr const auto max_args = 7;

namespace input_roll {
  struct node {
    hai::cstr data {};
    unsigned rpos = 0;
    hai::uptr<node> next {};
  };

  static hai::uptr<node> g_head {};

  void push(hai::cstr d) {
    g_head.reset(new node {
      .data = traits::move(d),
      .next = traits::move(g_head),
    });
  }

  char getc() {
    if (!g_head) return 0;

    auto c = g_head->data.begin()[g_head->rpos++];

    if (g_head->rpos >= g_head->data.size()) {
      g_head = traits::move(g_head->next);
    }

    return c;
  }

  bool empty() { return !g_head; }
}

namespace param_roll {
  hai::varray<char> g_data { 10240 };

  void push(char c) {
    g_data.push_back_doubling(c);
  }

  auto at(unsigned m) { return g_data.begin() + m; }
  auto end() { return g_data.end(); }

  auto mark() { return g_data.size(); }

  void dump() {
    for (auto c : g_data) put(c);
    putln();
  }
}

namespace storage_roll {
  hai::chain<char> g_data { 10240 };

  void push(char c) {
    g_data.push_back(c);
  }

  void dump() {
    for (auto c : g_data) put(c);
    putln();
  }
}

static jute::view after(jute::view v) {
  auto e = v.end() + 1;
  if (e > param_roll::end()) return {};
  if (!*e) return {};
  return jute::view::unsafe(e);
}

static hashley::fin<hai::varray<char>> g_mem { 127 }; 

static void ds(jute::view key) {
  auto val = after(key);
  hai::varray<char> mem { static_cast<unsigned>(val.size()) + 1 };
  for (auto i = 0; i < val.size(); i++) mem.push_back(val[i]);
  mem.push_back(0);
  g_mem[key] = traits::move(mem);
}

static void ss(jute::view key) {
  auto & val = g_mem[key];
  auto j = 0;
  for (auto i = 0; val[i]; i++, j++) {
    auto arg = after(key);
    char idx = 1;
    char c = val[i];
    while (arg.data()) {
      auto v = jute::view::unsafe(val.begin() + i).subview(arg.size()).before;
      if (v != arg) {
        arg = after(arg);
        idx++;
        if (idx == max_args) die("too many arguments to ss of ", key);
      } else {
        val[j] = idx;
        i += arg.size() - 1;
        idx = -1;
      }
    }
  }
  val[j] = 0;
}

static void call(jute::view fn, bool left) {
  jute::view args[max_args];

  unsigned i = 1;
  auto arg = after(fn);
  while (arg.begin()) {
    args[i++] = arg;
    arg = after(arg);
  }

  auto & data = g_mem[fn];
  if (left) {
    for (auto c : data) {
      if (c && c < max_args) {
        for (auto cc : args[static_cast<int>(c)]) param_roll::push(cc);
      } else {
        param_roll::push(c);
      }
    }
  } else {
    unsigned count = 0;
    for (auto c : data) {
      if (c && c < max_args) {
        count += args[static_cast<int>(c)].size();
      } else {
        count++;
      }
    }

    hai::cstr buf { count };
    auto ptr = buf.begin();
    for (auto c : data) {
      if (c && c < max_args) {
        for (auto cc : args[static_cast<int>(c)]) *ptr++ = cc;
      } else {
        *ptr++ = c;
      }
    }
    input_roll::push(traits::move(buf));
  }
}

static void run(unsigned mark, bool left) {
  auto fn = jute::view::unsafe(param_roll::at(mark));
  auto arg = after(fn);
  putln(fn, " ", arg);
  if      (fn == "ds") ds(arg);
  else if (fn == "ss") ss(arg);
  else if (fn.size())  call(fn, left);
  else die("trying to call an empty function");
}

static void parser();

static void parse_at() {
  switch (char c = input_roll::getc()) {
    case 0: break;
    default: param_roll::push(c); break;
  }
}

static void parse_dpound() {
  switch (char c = input_roll::getc()) {
    case '<': {
      auto mark = param_roll::mark();
      parser();
      if (!input_roll::empty()) run(mark, true);
      break;
    }
    default: 
      param_roll::push('#');
      param_roll::push('#');
      param_roll::push(c);
      break;
  }
}

static void parse_lt() {
  while (!input_roll::empty()) {
    switch (auto c = input_roll::getc()) {
      case 0:
        break;
      case '@':
        parse_at();
        break;
      case '>':
        return;
      case '<': 
        param_roll::push('<');
        parse_lt();
        if (!input_roll::empty()) param_roll::push('>');
        break;
      default:
        param_roll::push(c);
        break;
    }
  }
}

static void parse_pound() {
  switch (auto c = input_roll::getc()) {
    case '#':
      parse_dpound();
      break;
    case '<': {
      auto mark = param_roll::mark();
      parser();
      if (!input_roll::empty()) run(mark, false);
      break;
    }
    default: 
      param_roll::push('#');
      param_roll::push(c);
      break;
  }
};

static void parser() {
  while (!input_roll::empty()) {
    switch (auto c = input_roll::getc()) {
      case 0:  break;
      case '@': parse_at(); break;
      case '#': parse_pound(); break;
      case '<': parse_lt(); break;
      case ';': param_roll::push(0); break;
      case '>': param_roll::push(0); return;
      default:  param_roll::push(c); break;
    }
  }
}

int main() try {
  input_roll::push(jojo::read_cstr("example.ttm"));
  parser();
  param_roll::dump();
  storage_roll::dump();
} catch (...) {
  return 1;
}
