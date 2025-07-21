export module ttmc:roll;

namespace ttmc {
  class roll {
  public:
    constexpr virtual ~roll() {}
    constexpr virtual void push(char c) = 0;
  };
}
