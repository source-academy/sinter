display(eval_stream(null, -1));
display(eval_stream(null, 0));

let s = stream(1, 2, 3, 4, 5);
display(eval_stream(s, 0));
display(eval_stream(s, 1));
display(eval_stream(s, 2));
display(eval_stream(s, 3));
display(eval_stream(s, 4));
display(eval_stream(s, 5));

s = stream([1], [2], [3], [4], [5]);
display(eval_stream(s, 0));
display(eval_stream(s, 1));
display(eval_stream(s, 2));
display(eval_stream(s, 3));
display(eval_stream(s, 4));
display(eval_stream(s, 5));

null;
