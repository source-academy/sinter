const y = [1, [2, [3, [4, [5, null]]]]];
display(map(x => x * x, y));

null; // avoid testing returning array from toplevel
