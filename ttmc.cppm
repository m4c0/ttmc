export module ttmc;
import :inputroll;
import :memory;
import :paramroll;
import :state;
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

using namespace ttmc;

[[nodiscard]] static jute::heap numeric_binop(state * s, int (*op)(int, int)) {
  auto a = s->param.next();
  auto b = s->param.next();

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

[[nodiscard]] static jute::heap ad(state * s) {
  return numeric_binop(s, [](auto a, auto b) { return a + b; });
}

[[nodiscard]] static jute::heap cc(state * s) {
  return s->mem->consume(s->param.next(), 1);
}

static void ds(state * s) {
  s->mem->set(s->param.next(), s->param.next());
}

[[nodiscard]] static jute::heap eqn(state * s) {
  auto s1 = s->param.next();
  auto s2 = s->param.next();
  auto s3 = s->param.next();
  auto s4 = s->param.next();

  auto n1 = jute::to_i32(s1);
  auto n2 = jute::to_i32(s2);
  return n1 == n2 ? s3 : s4;
}

[[nodiscard]] static jute::heap eqq(state * s) {
  auto s1 = s->param.next();
  auto s2 = s->param.next();
  auto s3 = s->param.next();
  auto s4 = s->param.next();
  return s1 == s2 ? s3 : s4;
}

static void ss(state * s) {
  // TODO: support for substring priority by length
  //       i.e. #<ss;X;IS;THIS> matches THIS with higher precedence
  // TODO: introduce the "residual pointer" (note: keep "inital pointer")
  auto key = s->param.next();
  auto val = s->param.next();
  hai::varray<char> res { static_cast<unsigned>(val.size()) };
  for (auto i = 0; i < val.size(); i++) {
    auto ppr = s->param;
    auto arg = ppr.next();
    // TODO: start from the number of existing parameters
    //       i.e. #<ss;X;A;B>#<ss;X;C> is the same as #<ss;X;A;B;C>
    char idx = 1;
    char c = val[i];
    while (arg.data()) {
      auto v = val.subview(i, arg.size()).middle;
      if (v != arg) {
        arg = ppr.next();
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
  s->mem->set(key, jute::view { res.begin(), res.size() });
}

static void ps(state * s) { ttmc::printer(s->param.next()); }

[[nodiscard]] static jute::heap su(state * s) {
  return numeric_binop(s, [](auto a, auto b) { return a - b; });
}

[[nodiscard]] static jute::heap call(jute::view fn, state * s) {
  jute::view args[max_args];

  unsigned i = 1;
  auto arg = s->param.next();
  while (arg.begin()) {
    args[i++] = arg;
    arg = s->param.next();
  }

  const auto val = s->mem->get(fn);
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

[[nodiscard]] static jute::heap run(state * s) {
  // TODO: case insensitive

  auto fn = s->param.next();
  if      (fn == "ad")  return ad(s);
  else if (fn == "cc")  return cc(s);
  else if (fn == "ds")  ds(s);
  else if (fn == "eq")  return eqn(s);
  else if (fn == "eq?") return eqq(s);
  else if (fn == "ps")  ps(s);
  else if (fn == "ss")  ss(s);
  else if (fn == "su")  return su(s);
  else if (fn.size())   return call(fn, s);
  else assert(false && "trying to call an empty function");

  return {};
}

static void parser(state * s);

static void parse_at(state * s) {
  switch (char c = s->input->getc()) {
    case 0: break;
    default: s->param.push(c); break;
  }
}

static void parse_dpound(state * s) {
  switch (char c = s->input->getc()) {
    case '<': {
      state s2 = extend(s);
      parser(&s2);
      if (!s->input->empty()) s->param.push(*run(&s2));
      break;
    }
    default:
      s->param.push('#');
      s->param.push('#');
      s->param.push(c);
      break;
  }
}

static void parse_lt(state * s) {
  while (!s->input->empty()) {
    switch (auto c = s->input->getc()) {
      case 0:
        break;
      case '@':
        parse_at(s);
        break;
      case '>':
        return;
      case '<': 
        s->param.push('<');
        parse_lt(s);
        if (!s->input->empty()) s->param.push('>');
        break;
      default:
        s->param.push(c);
        break;
    }
  }
}

static void parse_pound(state * s) {
  switch (auto c = s->input->getc()) {
    case '#':
      parse_dpound(s);
      break;
    case '<': {
      state s2 = extend(s);
      parser(&s2);
      if (!s->input->empty()) s->input->push(*run(&s2));
      break;
    }
    default:
      s->param.push('#');
      s->param.push(c);
      break;
  }
};

static void parser(state * s) {
  while (!s->input->empty()) {
    switch (auto c = s->input->getc()) {
      case 0:    break;
      case '\n': break;
      case '@': parse_at(s); break;
      case '#': parse_pound(s); break;
      case '<': parse_lt(s); break;
      case ';': s->param.push('\0'); break;
      case '>': s->param.push('\0'); return;
      default:  s->param.push(c); break;
    }
  }
}

namespace ttmc {
  export jute::heap parse(jute::view file) {
    memory m {};
    inputroll in {};
    in.push(file);

    state s {
      .mem = &m,
      .input = &in,
    };

    while (!in.empty()) {
      switch (auto c = in.getc()) {
        case 0:   break;
        case '#': parse_pound(&s); break;
        default:  s.param.push(c); break;
      }
    }
  
    return s.param.rest();
  }
}
