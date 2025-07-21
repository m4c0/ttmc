export module ttmc;
import :inputroll;
import :paramroll;
import :memory;
import hai;
import hashley;
import jute;
import print;

using namespace jute::literals;

static constexpr const auto max_args = 7;

namespace ttmc {
  export hai::fn<void, jute::view> printer = [](auto m) { err(m); };

  static void assert_impl(const char * msg, const char * file, int line) {
    die(file, ":", line, ": ", msg);
  }
}

#define assert(X) do {               \
  if (!(X)) ttmc::assert_impl("assertion failed: " #X, __FILE__, __LINE__); \
} while (0)

namespace ttmc::input_roll {
  static inputroll g_in {};

  static void push(const char * data, unsigned sz) {
    g_in.push(jute::view { data, sz });
  }
  static void push(jute::view data) {
    g_in.push(data);
  }

  static auto empty() { return g_in.empty(); }
  static auto getc() { return g_in.getc(); }
};

namespace ttmc::param_roll {
  static constexpr const auto buf_size = 102400;
  static char g_data[buf_size] {};
  static unsigned g_ptr = 0;

  static void clear() { g_ptr = 0; }

  static void push(const char * c, unsigned sz) {
    assert(g_ptr + sz < buf_size && "parameter roll overflow");
    for (auto i = 0; i < sz; i++) g_data[g_ptr++] = *c++;
  }
  static void push(jute::view v) { push(v.begin(), v.size()); }
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

namespace ttmc::storage_roll {
  static constexpr const auto buf_size = 102400;
  char g_data[buf_size] {};
  unsigned g_ptr = 0;

  static void clear() { g_ptr = 0; }

  static void push(const char * c, unsigned n) {
    assert(g_ptr + n < buf_size && "storage roll overflow");
    for (auto i = 0; i < n; i++) g_data[g_ptr++] = c[i];
  }

  static auto data() { return jute::view { g_data, g_ptr }; }
}

namespace ttmc {
  using roll_t = void (*)(const char *, unsigned);

  void clear() {
    input_roll::g_in = {};
    param_roll::clear();
    storage_roll::clear();
  }
}

using namespace ttmc;

static memory g_mem {};

static jute::view after(jute::view v) {
  if (!v.begin()) return {};

  auto e = v.end() + 1;
  if (e >= param_roll::end()) return {};
  return jute::view::unsafe(e);
}

[[nodiscard]] static jute::heap numeric_binop(jute::view a, int (*op)(int, int)) {
  auto b = after(a);

  auto ab = op(jute::to_i32(a), jute::to_i32(b));
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
  return g_mem.consume(key, 1);
}

static void ds(jute::view key) {
  g_mem.set(key, after(key));
}

[[nodiscard]] static jute::heap eqn(jute::view s1) {
  auto s2 = after(s1);
  auto s3 = after(s2);
  auto s4 = after(s3);

  auto n1 = jute::to_i32(s1);
  auto n2 = jute::to_i32(s2);
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
  auto val = g_mem.get(key);
  hai::varray<char> res { static_cast<unsigned>(val.size()) };
  for (auto i = 0; i < val.size(); i++) {
    auto arg = after(key);
    // TODO: start from the number of existing parameters
    //       i.e. #<ss;X;A;B>#<ss;X;C> is the same as #<ss;X;A;B;C>
    char idx = 1;
    char c = val[i];
    while (arg.data()) {
      auto v = val.subview(i, arg.size()).middle;
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
    res.push_back(c);
  }
  g_mem.set(key, jute::view { res.begin(), res.size() });
}

static void ps(jute::view arg) { ttmc::printer(arg); }

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

  const auto val = g_mem.get(fn);
  if (val == "") {
    errln("warn: missing definition of #<", fn, ">");
    return {};
  }

  char buf[300] {};
  unsigned idx = 0;
  for (auto c : val) {
    if (!c || c >= max_args) {
      if (idx >= sizeof(buf)) die("overflow from ", fn, ": ", jute::view{buf});
      buf[idx++] = c;
      continue;
    }
    auto a = args[static_cast<int>(c)];
    for (auto c : a) {
      if (idx >= sizeof(buf)) die("overflow from ", fn, ": ", jute::view{buf});
      buf[idx++] = c;
    }
  }
  return jute::view { buf, idx };
}

[[nodiscard]] static jute::heap run(unsigned mark) {
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
  return res;
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
      if (!input_roll::empty()) param_roll::push(*run(mark));
      break;
    }
    default:
      roll("##", 2);
      roll(&c, 1);
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
      if (!input_roll::empty()) input_roll::push(*run(mark));
      break;
    }
    default:
      roll("#", 1);
      roll(&c, 1);
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

namespace ttmc {
  export jute::view parse(jute::view file) {
    clear();
    g_mem = {};

    input_roll::push(file.begin(), file.size());
  
    while (!input_roll::empty()) {
      switch (auto c = input_roll::getc()) {
        case 0:   break;
        case '#': parse_pound(storage_roll::push); break;
        default:  storage_roll::push(&c, 1); break;
      }
    }
  
    return storage_roll::data();
  }
}
