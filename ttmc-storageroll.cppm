export module ttmc:storageroll;
import :roll;
import hai;
import jute;

namespace ttmc {
  class storageroll;
}

class ttmc::storageroll : public ttmc::roll {
  hai::varray<char> m_data { 1024 };
public:
  constexpr void push(char c) override { m_data.push_back_doubling(c); }

  constexpr jute::view data() const {
    return { m_data.begin(), m_data.size() };
  }
};
