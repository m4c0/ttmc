#pragma leco tool

import hai;
import hashley;
import jojo;
import jute;
import print;

using namespace jute::literals;

static constexpr const auto max_args = 7;

namespace ttmc {
  static void assert_impl(const char * msg, const char * file, int line) {
    die(file, ":", line, ": ", msg);
  }
}

#define assert(X) do {               \
  if (!(X)) ttmc::assert_impl("assertion failed: " #X " around ", __FILE__, __LINE__); \
} while (0)

namespace ttmc::input_roll {
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
    if (c != 0) return c;

    auto tmp = g_head;
    g_head = g_head->next;
    delete[] tmp->data;
    delete tmp;

    return getc();
  }

  static bool empty() { return !g_head; }

  static void dump() {
    auto * n = &*g_head;
    while (n) {
      put(&n->data[n->rpos]);
      n = &*(n->next);
    }
    putln();
  }
};

namespace ttmc::param_roll {
  static constexpr const auto buf_size = 102400;
  static char g_data[buf_size] {};
  static unsigned g_ptr = 0;

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

  static void dump() { putln(jute::view { g_data, g_ptr }); }
};

namespace ttmc::storage_roll {
  static constexpr const auto buf_size = 102400;
  char g_data[buf_size] {};
  unsigned g_ptr = 0;

  void push(const char * c, unsigned n) {
    assert(g_ptr + n < buf_size && "storage roll overflow");
    for (auto i = 0; i < n; i++) g_data[g_ptr++] = c[i];
  }

  void dump() {
    putln(jute::view { g_data, g_ptr });
  }
}

namespace ttmc {
  using roll_t = void (*)(const char *, unsigned);

  [[maybe_unused]] void dump_all() {
    putln("-=-=-=-=-=-=-=-=-=- Input Roll");
    input_roll::dump();
    putln("-=-=-=-=-=-=-=-=-=- Parameter Roll");
    param_roll::dump();
    putln("-=-=-=-=-=-=-=-=-=- Storage Roll");
    storage_roll::dump();
    putln("-=-=-=-=-=-=-=-=-=-");
  }
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Anything below requires some API cleanup
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace memory {
  struct node {
    char * data;
    unsigned size;
    unsigned r_pos = 0;
  };
  static hashley::fin<node> g_data { 127 }; 

  static bool has(jute::view key) {
    return g_data.has(key);
  }

  static node & get(jute::view key) {
    assert(has(key) && "trying to fetch non-existant key");
    return g_data[key];
  }

  static void set(jute::view key, const char * val, unsigned sz) {
    auto & n = g_data[key];
    if (n.data) delete[] n.data;

    auto data = new char[sz];
    for (auto i = 0; i < sz; i++) data[i] = val[i];

    n.data = data;
    n.size = sz;
    n.r_pos = 0;
  }
}

using namespace ttmc;

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

[[nodiscard]] static jute::heap numeric_binop(jute::view a, int (*op)(int, int)) {
  auto b = after(a);

  auto ab = op(str_to_i32(a), str_to_i32(b));
  if (ab == 0) return "0"_hs;

  char buf[128] {};
  auto p = buf;
  if (ab < 0) {
    *p++ = '-';
    ab = -ab;
  }
  auto n = ab;
  while (n > 0) {
    p++;
    n /= 10;
  }
  unsigned len = p - buf;
  assert(len < sizeof(buf) && "buffer overflow converting number to string");
  while (ab > 0) {
    *--p = '0' + (ab % 10);
    ab /= 10;
  }

  return jute::view { buf, len };
}

[[nodiscard]] static jute::heap ad(jute::view a) {
  return numeric_binop(a, [](auto a, auto b) { return a + b; });
}

[[nodiscard]] static jute::heap cc(jute::view key) {
  if (!memory::has(key)) return {};

  auto & val = memory::get(key);
  if (val.r_pos >= val.size) return {};

  return jute::view { &val.data[val.r_pos++], 1 };
}

static void ds(jute::view key) {
  auto val = after(key);
  memory::set(key, val.begin(), val.size());
}

[[nodiscard]] static jute::heap eqn(jute::view s1) {
  auto s2 = after(s1);
  auto s3 = after(s2);
  auto s4 = after(s3);

  auto n1 = str_to_i32(s1);
  auto n2 = str_to_i32(s2);
  return n1 == n2 ? s3 : s4;
}

[[nodiscard]] static jute::heap eqq(jute::view s1) {
  auto s2 = after(s1);
  auto s3 = after(s2);
  auto s4 = after(s3);
  return s1 == s2 ? s3 : s4;
}

static void ss(jute::view key) {
  // TODO: support for substring priority by length
  //       i.e. #<ss;X;IS;THIS> matches THIS with higher precedence
  // TODO: introduce the "residual pointer" (note: keep "inital pointer")
  auto & val = memory::get(key);
  auto j = 0;
  for (auto i = 0; val.data[i]; i++, j++) {
    auto arg = after(key);
    // TODO: start from the number of existing parameters
    //       i.e. #<ss;X;A;B>#<ss;X;C> is the same as #<ss;X;A;B;C>
    char idx = 1;
    char c = val.data[i];
    while (arg.data()) {
      auto v = jute::view::unsafe(&val.data[i]).subview(arg.size()).before;
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
  val.size = j;
}

static void ps(jute::view arg) { errln(arg); }

[[nodiscard]] static jute::heap su(jute::view a) {
  return numeric_binop(a, [](auto a, auto b) { return a - b; });
}

[[nodiscard]] static jute::heap call(jute::view fn) {
  jute::view args[max_args];

  unsigned i = 1;
  auto arg = after(fn);
  while (arg.begin()) {
    args[i++] = arg;
    arg = after(arg);
  }

  if (!memory::has(fn)) {
    errln("warn: missing definition of #<", fn, ">");
    return {};
  }

  // TODO: should we consider "residuals" here?
  const auto val = memory::get(fn);
  char buf[300] {};
  unsigned idx = 0;
  for (auto i = 0; i < val.size; i++) {
    auto c = val.data[i];
    if (!c || c >= max_args) {
      if (idx >= sizeof(buf)) die("function name overflow");
      buf[idx++] = c;
      continue;
    }
    auto a = args[static_cast<int>(c)];
    for (auto c : a) {
      if (idx >= sizeof(buf)) die("function name overflow");
      buf[idx++] = c;
    }
  }
  return jute::view { buf, idx };
}

static void run(unsigned mark, roll_t roll) {
  // TODO: case insensitive

  jute::heap res {};
  auto fn = jute::view::unsafe(param_roll::at(mark));
  auto arg = after(fn);
  if      (fn == "ad")  res = ad(arg);
  else if (fn == "cc")  res = cc(arg);
  else if (fn == "ds")  ds(arg);
  else if (fn == "eq")  res = eqn(arg);
  else if (fn == "eq?") res = eqq(arg);
  else if (fn == "ps")  ps(arg);
  else if (fn == "ss")  ss(arg);
  else if (fn == "su")  res = su(arg);
  else if (fn.size())   res = call(fn);
  else assert(false && "trying to call an empty function");

  param_roll::truncate_at(mark);
  if (res.size() != 0) roll(res.begin(), res.size());
}

static void parser();

static void parse_at() {
  switch (char c = input_roll::getc()) {
    case 0: break;
    default: param_roll::push(c); break;
  }
}

static void parse_dpound(roll_t roll) {
  switch (char c = input_roll::getc()) {
    case '<': {
      auto mark = param_roll::mark();
      parser();
      if (!input_roll::empty()) run(mark, param_roll::push);
      break;
    }
    default:
      roll("##", 2);
      input_roll::push(&c, 1);
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

static void parse_pound(roll_t roll) {
  switch (auto c = input_roll::getc()) {
    case '#':
      parse_dpound(roll);
      break;
    case '<': {
      auto mark = param_roll::mark();
      parser();
      if (!input_roll::empty()) run(mark, input_roll::push);
      break;
    }
    default:
      roll("#", 1);
      input_roll::push(&c, 1);
      break;
  }
};

static void parser() {
  while (!input_roll::empty()) {
    switch (auto c = input_roll::getc()) {
      case 0:    break;
      case '\n': break;
      case '@': parse_at(); break;
      case '#': parse_pound(param_roll::push); break;
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
      case '#': parse_pound(storage_roll::push); break;
      default:  storage_roll::push(&c, 1); break;
    }
  }

  storage_roll::dump();
}

int main(int argc, char ** argv) try {
  for (auto i = 1; i < argc; i++) parse_file(argv[i]);
} catch (...) {
  return 1;
}
