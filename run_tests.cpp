#pragma leco tool

import jute;
import pprent;
import print;

int main() {
  for (auto f : pprent::list("examples")) {
    auto file = jute::view::unsafe(f);
    if (!file.ends_with(".ttm")) continue;

    auto ttm = ("examples/" + file).cstr();
    auto txt = (jute::view{ttm}.rsplit('.').before + ".txt").cstr();

    putln(ttm, " ", txt);
  }
}
