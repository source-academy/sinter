display(1 === 1);
display(1 !== 1);
display(1 === 1.0);
display(1 !== 1.0);
display("a" === "a");
display("a" !== "a");
display("a" === "b");
display("a" !== "b");
display([] === []);
display([] !== []);
display(null === null);
display(null !== null);
display(undefined === undefined);
display(undefined !== undefined);

let x = 1;
let y = 2;
display(x === y);
display(x !== y);

y = 1;
display(x === y);
display(x !== y);

display([1] === [1]);
display([1] !== [1]);

display([1] === [2]);
display([1] !== [2]);

x = [1];
y = x;
display(x === y);
display(x !== y);

display(true === true);
display(true !== true);
display(true === false);
display(true !== false);
display(false === false);
display(false !== false);
display(false === true);
display(false !== true);
display(NaN === NaN);
display(NaN !== NaN);
display(Infinity === Infinity);
display(Infinity !== Infinity);

display(1 === 2.0 / 2);
display(2.0 / 2 === 1);
display(5.5 === 6.5);
