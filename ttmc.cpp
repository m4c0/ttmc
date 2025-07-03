#pragma leco tool

import hai;
import hashley;
import jojo;
import jute;
import print;

static constexpr const auto max_args = 7;

struct input_roll {
  struct node {
    hai::cstr data {};
    unsigned rpos = 0;
    hai::uptr<node> next {};
  };

  static hai::uptr<node> g_head;

  static void push(hai::cstr d) {
    g_head.reset(new node {
      .data = traits::move(d),
      .next = traits::move(g_head),
    });
  }
  static void push(jute::view d) { push(d.cstr()); }

  static char getc() {
    if (!g_head) return 0;

    auto c = g_head->data.begin()[g_head->rpos++];

    if (g_head->rpos >= g_head->data.size()) {
      auto tmp = traits::move(g_head->next);
      g_head = traits::move(tmp);
    }

    return c;
  }

  static bool empty() { return !g_head; }

  static void dump() {
    auto * n = &*g_head;
    while (n) {
      put(n->data.begin() + n->rpos);
      n = &*(n->next);
    }
    putln();
  }
};
hai::uptr<input_roll::node> input_roll::g_head {};

struct param_roll {
  static hai::varray<char> g_data;

  static void push(char c) {
    if (g_data.size() == g_data.capacity()) die("parameter roll overflow");
    g_data.push_back(c);
  }
  static void push(hai::cstr d) { for (auto c : d) push(c); }
  static void push(jute::view d) { for (auto c : d) push(c); }

  static auto at(unsigned m) { return g_data.begin() + m; }
  static auto end() { return g_data.end(); }

  static auto mark() { return g_data.size(); }

  static void truncate_at(unsigned m) { g_data.truncate(m); }

  static void dump() {
    putln(jute::view { g_data.begin(), g_data.size() });
  }
};
hai::varray<char> param_roll::g_data { 102400 };

namespace storage_roll {
  hai::varray<char> g_data { 102400 };

  void push(char c) {
    if (g_data.size() == g_data.capacity()) die("storage roll overflow");
    g_data.push_back(c);
  }

  void dump() {
    putln(jute::view { g_data.begin(), g_data.size() });
  }
}

template<typename T>
concept roll = requires (jute::view v, hai::cstr c) {
  T::push(v);
  T::push(traits::move(c));
};

static jute::view after(jute::view v) {
  if (!v.begin()) return {};

  auto e = v.end() + 1;
  if (e > param_roll::end()) return {};
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

static void eqq(jute::view s1, roll auto roll) {
  auto s2 = after(s1);
  auto s3 = after(s2);
  auto s4 = after(s3);
  roll.push(s1 == s2 ? s3 : s4);
}

static void ss(jute::view key) {
  // TODO: support for substring priority by length
  //       i.e. #<ss;X;IS;THIS> matches THIS with higher precedence
  // TODO: introduce the "residual pointer" (note: keep "inital pointer")
  auto & val = g_mem[key];
  auto j = 0;
  for (auto i = 0; val[i]; i++, j++) {
    auto arg = after(key);
    // TODO: start from the number of existing parameters
    //       i.e. #<ss;X;A;B>#<ss;X;C> is the same as #<ss;X;A;B;C>
    char idx = 1;
    char c = val[i];
    while (arg.data()) {
      auto v = jute::view::unsafe(val.begin() + i).subview(arg.size()).before;
      if (v != arg) {
        arg = after(arg);
        idx++;
        if (idx == max_args) die("too many arguments to ss of ", key);
      } else {
        c = idx;
        i += arg.size() - 1;
        break;
      }
    }
    val[j] = c;
  }
  val[j] = 0;
  val.truncate(j);
}

static void ps(jute::view arg) { put(arg); }

static void call(jute::view fn, roll auto roll) {
  jute::view args[max_args];

  unsigned i = 1;
  auto arg = after(fn);
  while (arg.begin()) {
    args[i++] = arg;
    arg = after(arg);
  }

  if (!g_mem.has(fn)) {
    errln("missing definition of #<", fn, ">");
    return;
  }

  const auto & data = g_mem[fn];
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

  roll.push(traits::move(buf));
}

static void run(unsigned mark, roll auto roll) {
  auto fn = jute::view::unsafe(param_roll::at(mark));
  auto arg = after(fn);
  if      (fn == "ds")  ds(arg);
  else if (fn == "eq?") eqq(arg, roll);
  else if (fn == "ps")  ps(arg);
  else if (fn == "ss")  ss(arg);
  else if (fn.size())  call(fn, roll);
  else die("trying to call an empty function");

  param_roll::truncate_at(mark);
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
      if (!input_roll::empty()) run(mark, param_roll {});
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
      if (!input_roll::empty()) run(mark, input_roll {});
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
      case 0:   break;
      case '@': parse_at(); break;
      case '#': parse_pound(); break;
      case '<': parse_lt(); break;
      case ';': param_roll::push('\0'); break;
      case '>': param_roll::push('\0'); return;
      default:  param_roll::push(c); break;
    }
  }
}

static void parse_file(const char * name) {
  input_roll::push(jojo::read_cstr(jute::view::unsafe(name)));

  while (!input_roll::empty()) {
    switch (auto c = input_roll::getc()) {
      case 0:   break;
      case '#': parse_pound(); break;
      default:  storage_roll::push(c); break;
    }
  }

  storage_roll::dump();
}

int main(int argc, char ** argv) try {
  for (auto i = 1; i < argc; i++) parse_file(argv[i]);
} catch (...) {
  return 1;
}
