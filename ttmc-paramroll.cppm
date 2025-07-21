export module ttmc:paramroll;
import :roll;
import hai;
import jute;

namespace ttmc {
  class paramroll;
}

class ttmc::paramroll : public ttmc::roll {
  jute::heap m_data {}; 
  jute::view m_view {};

public:
  constexpr void push(char c) override {
    m_data = m_data + c;
    m_view = *m_data;
  }
  constexpr void push(jute::view v) {
    m_data = m_data + v;
    m_view = *m_data;
  }

  constexpr jute::view next() {
    auto [l, r] = m_view.split(0);
    m_view = r;
    return l;
  }
};

static_assert([] {
  ttmc::paramroll p {};
  p.push("this\0is\0fine");
  p.push('!');

  if (p.next() != "this") throw 0;
  if (p.next() != "is") throw 0;
  auto pp = p;
  if (p.next() != "fine!") throw 0;
  if (p.next().data()) throw 0;

  if (pp.next() != "fine!") throw 0;
  if (pp.next().data()) throw 0;

  return true;
}());
