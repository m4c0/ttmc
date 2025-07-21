export module ttmc:inputroll;
import jute;
import hai;

namespace ttmc {
  class inputroll;
}

class ttmc::inputroll {
  struct node {
    jute::heap data {};
    unsigned rpos = 0;
    hai::uptr<node> next {};
  };

  hai::uptr<node> m_head {};

public:
  constexpr void push(jute::heap data) {
    m_head.reset(new node {
      .data = data,
      .next = traits::move(m_head),
    });
  }

  constexpr char getc() {
    if (!m_head) return 0;

    char c = (*m_head->data)[m_head->rpos++];

    if (m_head->rpos >= m_head->data.size()) {
      auto tmp = traits::move(m_head->next);
      m_head = traits::move(tmp);
    }

    return c;
  }

  constexpr bool empty() { return !m_head; }
};

static_assert([] {
  using namespace jute::literals;

  ttmc::inputroll in {};
  in.push("ok!"_hs);
  in.push("is "_hs);
  in.push("it "_hs);

  const auto take = [&](char c) {
    if (in.empty()) throw 0;
    if (in.getc() != c) throw 0;
  };

  take('i'); take('t'); take(' ');
  take('i'); take('s'); take(' ');
  take('o'); take('k'); take('!');

  if (!in.empty()) throw 0;
  return in.getc() == 0;
}());
