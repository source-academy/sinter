function a(x) {
  return () => x;
}

let c = 0;
for (let i = 0; i < 10000; i = i + 1) {
  c = c + a(i)();
}

display(c);
