#pragma leco tool

import hai;
import hashley;
import jojo;
import jute;
import print;

static constexpr const auto max_args = 7;

static void assert__(const char * msg, const char * file, int line) {
  die(file, ":", line, ": ", msg);
}
#define assert(X) do {               \
  if (!(X)) assert__("assertion failed: " #X " around ", __FILE__, __LINE__); \
} while (0)

namespace input_roll {
  struct node;

  struct node {
    const char * data;
    unsigned rpos = 0;
    node * next {};
  };

  static node * g_head {};

  static void push(const char * data, unsigned sz) {
    auto d = new char[sz] {};
    for (auto i = 0; i < sz; i++) d[i] = data[i];
    g_head = new node {
      .data = d,
      .next = g_head,
    };
  }

  static char getc() {
    if (!g_head) return 0;

    char c = g_head->data[g_head->rpos++];
    if (c == 0) {
      auto tmp = g_head;
      g_head = g_head->next;
      delete tmp->data;
      delete tmp;
    }

    return c;
  }

  static bool empty() { return !g_head; }

  [[maybe_unused]]
  static void dump() {
    auto * n = &*g_head;
    while (n) {
      put(&n->data[n->rpos]);
      n = &*(n->next);
    }
    putln();
  }
};

namespace param_roll {
  static constexpr const auto buf_size = 102400;
  static char g_data[buf_size] {};
  static unsigned g_ptr = 0;

  [[maybe_unused]]
  static void dump() { putln(jute::view { g_data, g_ptr }); }

  static void push(const char * c, unsigned sz) {
    assert(g_ptr + sz < buf_size && "parameter roll overflow");
    for (auto i = 0; i < sz; i++) g_data[g_ptr++] = *c++;
  }
  static void push(char c) { push(&c, 1); }

  static auto at(unsigned m) {
    assert(m < buf_size && "parameter roll out-of-bounds access");
    return &g_data[m];
  }
  static auto end() { return &g_data[g_ptr]; }

  static auto mark() { return g_ptr; }

  static void truncate_at(unsigned m) {
    assert(m < buf_size && "parameter roll out-of-bounds truncation");
    assert(m <= g_ptr && "parameter roll truncation beyond read point");
    g_ptr = m;
  }
};

namespace storage_roll {
  hai::varray<char> g_data { 102400 };

  void push(char c) {
    assert(g_data.size() < g_data.capacity() && "storage roll overflow");
    g_data.push_back(c);
  }

  void dump() {
    putln(jute::view { g_data.begin(), g_data.size() });
  }
}

using roll_t = void (*)(const char *, unsigned);

static jute::view after(jute::view v) {
  if (!v.begin()) return {};

  auto e = v.end() + 1;
  if (e >= param_roll::end()) return {};
  return jute::view::unsafe(e);
}

static constexpr int str_to_i32(jute::view v) {
  int sign = (v.size() && v[0] == '-') ? -1 : 1;
  if (sign == -1) v = v.subview(1).after;
  int res = 0;
  for (auto c : v) {
    if (c < '0' || c > '9') break;
    res = res * 10 + (c - '0');
  }
  return res * sign;
}
static_assert(str_to_i32("-23") == -23);
static_assert(str_to_i32("948") == 948);

struct mem_element {
  hai::varray<char> data {};
  unsigned r_pos = 0;
};
static hashley::fin<mem_element> g_mem { 127 }; 

static void cc(jute::view key, roll_t roll) {
  if (!g_mem.has(key)) return;

  auto & val = g_mem[key];
  jute::view c {};
  if (val.r_pos < val.data.size()) c = { val.data.begin() + val.r_pos++, 1 };
  roll(c.begin(), c.size());
}

static void ds(jute::view key) {
  auto val = after(key);
  hai::varray<char> mem { static_cast<unsigned>(val.size()) + 1 };
  for (auto i = 0; i < val.size(); i++) mem.push_back(val[i]);
  mem.push_back(0);
  g_mem[key] = { traits::move(mem) };
}

static void eqn(jute::view s1, roll_t roll) {
  auto s2 = after(s1);
  auto s3 = after(s2);
  auto s4 = after(s3);

  auto n1 = str_to_i32(s1);
  auto n2 = str_to_i32(s2);
  auto res = n1 == n2 ? s3 : s4;
  roll(res.begin(), res.size());
}

static void eqq(jute::view s1, roll_t roll) {
  auto s2 = after(s1);
  auto s3 = after(s2);
  auto s4 = after(s3);
  auto res = s1 == s2 ? s3 : s4;
  roll(res.begin(), res.size());
}

static void ss(jute::view key) {
  // TODO: support for substring priority by length
  //       i.e. #<ss;X;IS;THIS> matches THIS with higher precedence
  // TODO: introduce the "residual pointer" (note: keep "inital pointer")
  auto & val = g_mem[key];
  auto j = 0;
  for (auto i = 0; val.data[i]; i++, j++) {
    auto arg = after(key);
    // TODO: start from the number of existing parameters
    //       i.e. #<ss;X;A;B>#<ss;X;C> is the same as #<ss;X;A;B;C>
    char idx = 1;
    char c = val.data[i];
    while (arg.data()) {
      auto v = jute::view::unsafe(val.data.begin() + i).subview(arg.size()).before;
      if (v != arg) {
        arg = after(arg);
        idx++;
        assert(idx < max_args && "too many arguments for ss");
      } else {
        c = idx;
        i += arg.size() - 1;
        break;
      }
    }
    val.data[j] = c;
  }
  val.data[j] = 0;
  val.data.truncate(j);
}

static void ps(jute::view arg) { put(arg); }

static void call(jute::view fn, roll_t roll) {
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

  // TODO: should we consider "residuals" here?
  const auto & data = g_mem[fn].data;
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

  roll(buf.begin(), buf.size());
}

static void run(unsigned mark, roll_t roll) {
  // TODO: case insensitive

  auto fn = jute::view::unsafe(param_roll::at(mark));
  auto arg = after(fn);
  if      (fn == "cc")  cc(arg, roll);
  else if (fn == "ds")  ds(arg);
  else if (fn == "eq")  eqn(arg, roll);
  else if (fn == "eq?") eqq(arg, roll);
  else if (fn == "ps")  ps(arg);
  else if (fn == "ss")  ss(arg);
  else if (fn.size())  call(fn, roll);
  else assert(false && "trying to call an empty function");

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
      if (!input_roll::empty()) run(mark, param_roll::push);
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
      if (!input_roll::empty()) run(mark, input_roll::push);
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
      case 0:    break;
      case '\n': break;
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
  auto file = jojo::read_cstr(jute::view::unsafe(name));
  input_roll::push(file.begin(), file.size());

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
