display(remove_all(1, list(1, 2, 1, 3, 1, 4, 1, 5)));
display(remove_all(2, list(1, 2, 1, 3, 1, 4, 1, 5)));
display(remove_all(2, list(1, 2, 1, 2, 2, 4, 1, 5)));
display(remove_all(2, list(1, 2, 1, 2, 2, 4, 2, 5)));
display(remove_all(2, list(2)));
display(remove_all(2, null));
display(remove_all(2, list(2, 2, 2, 2, 2)));

let x = [null, null];
let y = [x, [x, [x, null]]];
x[1] = y;
x[0] = x;

display(remove_all(x, x));

null;
