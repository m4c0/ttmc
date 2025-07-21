export module ttmc:memory;
import hai;
import hashley;
import jute;

export class memory {
  struct node {
    hai::array<char> data {};
    unsigned r_pos = 0;
  };

  hashley::fin<node> m_data { 127 };

public:
  constexpr jute::view consume(jute::view key, unsigned n) {
    if (!m_data.has(key)) return {};

    auto & val = m_data[key];
    if (n + val.r_pos >= val.data.size()) return {};

    jute::view res { val.data.begin() + val.r_pos, n };
    val.r_pos += n;
    return res;
  }

  constexpr jute::view get(jute::view key) {
    if (!m_data.has(key)) return {};
    auto & v = m_data[key];
    // TODO: consider residual pointer?
    return jute::view { v.data.begin(), v.data.size() };
  }

  constexpr void set(jute::view key, jute::view data) {
    auto & n = m_data[key];
    n = { .data { static_cast<unsigned>(data.size()) } };
    for (auto i = 0; i < data.size(); i++) n.data[i] = data[i];
  }
};
