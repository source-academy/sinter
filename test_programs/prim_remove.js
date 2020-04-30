display(remove(1, list(1, 2, 3, 4, 5)));
display(remove(2, list(1, 2, 3, 4, 5)));
display(remove(5, list(1, 2, 3, 4, 5)));
display(remove(50, list(1, 2, 3, 4, 5)));

let x = [];

display(remove(x, list([], x, 3, [], 4)));

display(remove(1, null));

let y = [1, null];
y[0] = y;
y[1] = y;
display(remove(y, y));

null;
