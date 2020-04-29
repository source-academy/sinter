const y = list(1, 2, 3, 4, 5);
display(y);
const z = map(x => [x], y);
display(z);
const a = map(x => x[0], z);
display(a);

null;
