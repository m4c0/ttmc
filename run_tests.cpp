#pragma leco tool

import jojo;
import jute;
import mtime;
import pprent;
import print;
import ttmc;

using namespace jute::literals;

int main() {
  for (auto f : pprent::list("examples")) {
    auto file = jute::view::unsafe(f);
    if (!file.ends_with(".ttm")) continue;

    auto ttm = ("examples/" + file).cstr();
    auto txt = (jute::view{ttm}.rsplit('.').before + ".txt").cstr();
    jute::heap result = "Printer:\n"_hs;
    try {
      putln("[run  ] ", ttm);
      ttmc::printer = [&](auto msg) { result = result + msg + "\n"; };
      auto output = ttmc::parse(jojo::read_cstr(ttm));

      result = result + "Output:\n";
      auto rest = *output;
      while (rest.size()) {
        // Ignore empty lines for test output brevity
        auto [l, r] = rest.split('\n');
        if (l != "") result = result + l + "\n";
        rest = r;
      }

      if (!mtime::of(txt.begin())) continue;
      auto expected = jojo::read_cstr(txt);
      if (result != jute::view{expected}) {
        errln("[failed] ", ttm);
        errln("Expected:");
        errln(expected);
        errln("Got:");
        errln(result);
      }
    } catch (...) {
      errln("[erred] ", ttm);
    }
  }
}
