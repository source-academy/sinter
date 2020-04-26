function a(x) {
  let f = () => x;
  return f;
}

let c = 0;
for (let i = 0; i < 10000; i = i + 1) {
  c = c + a(i)();
}

display(c);
