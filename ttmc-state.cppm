export module ttmc:state;
import :inputroll;
import :memory;
import :paramroll;

namespace ttmc {
  struct state {
    memory * mem;
    inputroll * input;
    paramroll param {};
  };

  state extend(state * s) {
    return {
      .mem = s->mem,
      .input = s->input,
    };
  }
}
