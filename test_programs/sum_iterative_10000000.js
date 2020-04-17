function add(a, b) {
  return a <= 0 ? b : add(a - 1, b + 1);
}

add(42, 9999958);
