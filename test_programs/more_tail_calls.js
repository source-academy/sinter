function tc(f, x) {
  return f(x);
}

tc(display, 1);
tc(display, (f => f())(stream([1], [2])[1]));

null;
