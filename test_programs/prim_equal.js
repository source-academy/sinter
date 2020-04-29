display(equal(1, 1));
display(equal(1, 1.0));
display(equal("a", "a"));
display(equal("a", "b"));
display(equal([], []));
display(equal(null, null));
display(equal(undefined, undefined));

let x = 1;
let y = 2;
display(equal(x, y));

y = 1;
display(equal(x, y));

display(equal([1], [1]));

display(equal([1], [2]));

x = [1];
y = x;
display(equal(x, y));

display(equal(true, true));
display(equal(true, false));
display(equal(false, false));
display(equal(false, true));
display(equal(NaN, NaN));
display(equal(Infinity, Infinity));

x = [1, 2];
y = x;
display(equal(x, y));
display(equal([1, 2], [1, 2]));
display(equal([1, 1], [1, 2]));
display(equal([1, 1], null));
display(equal([1, 1], null));
