let x = [1, 2, 3, 4];
let y = [5, 6, 7, 8];
x[15] = 1234;
for (let i = 0; i < 15; i = i + 1) {
  x[i] = i < 4 ? y[i] : i;
}

for (let i = 0; i < 16; i = i + 1) {
  display(x[i]);
}
for (let i = 0; i < 4; i = i + 1) {
  display(y[i]);
}
