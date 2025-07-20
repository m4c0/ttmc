#pragma leco tool

import jojo;
import jute;
import pprent;
import print;
import ttmc;

int main() {
  for (auto f : pprent::list("examples")) {
    auto file = jute::view::unsafe(f);
    if (!file.ends_with(".ttm")) continue;

    auto ttm = ("examples/" + file).cstr();
    auto txt = (jute::view{ttm}.rsplit('.').before + ".txt").cstr();
    try {
      ttmc::parse(jojo::read_cstr(ttm));
    } catch (...) {
      putln("[erred] ", ttm);
    }
  }
}
