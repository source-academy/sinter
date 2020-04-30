display(append(null, null));
display(append(null, 1));
display(append(null, [1, [2, [3, null]]]));
display(append([1, [2, [3, null]]], [4, [5, [6, null]]]));

let x = [5, [6, null]];
let y = append([1, [4, null]], x);
display(tail(tail(y)) === x);

display(append(list(1, 2, 3), list(4, 5, 6)));
display(append(list(1, 2, 3), list(list(4), list(5), list(6))));

null;
