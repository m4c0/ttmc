export module ttmc:memory;
import hai;
import hashley;
import jute;

namespace ttmc {
  class memory;
}

class ttmc::memory {
  struct node {
    hai::array<char> data {};
    unsigned r_pos = 0;
  };

  hashley::fin<node> m_data { 127 };

public:
  constexpr jute::view consume(jute::view key, unsigned n) {
    if (!m_data.has(key)) return {};

    auto & val = m_data[key];
    if (n + val.r_pos > val.data.size()) return {};

    jute::view res { val.data.begin() + val.r_pos, n };
    val.r_pos += n;
    return res;
  }

  constexpr jute::view get(jute::view key) {
    if (!m_data.has(key)) return {};
    auto & v = m_data[key];
    return jute::view { v.data.begin() + v.r_pos, v.data.size() - v.r_pos };
  }

  constexpr void set(jute::view key, jute::view data) {
    auto & n = m_data[key];
    n = { .data { static_cast<unsigned>(data.size()) } };
    for (auto i = 0; i < data.size(); i++) n.data[i] = data[i];
  }
};

static_assert([] {
  ttmc::memory m {};
  m.set("X", "OK");

  if (m.consume("X", 1) != "O") throw 0;
  if (m.get("X") != "K") throw 0;
 
  if (m.consume("X", 1) != "K") throw 0;
  if (m.get("X") != "") throw 0;
 
  if (m.consume("X", 1) != "") throw 0;

  return true;
}());
