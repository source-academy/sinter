for_each(display, null);
for_each(display, list(1, 2, 3, 4, 5));
for_each(display, list([1], [2], [3], [4], [5]));
for_each(x => display(x[0] * x[0]), list([1], [2], [3], [4], [5]));

null;
