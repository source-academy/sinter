let x = list_to_stream(list(1, 2, 3));
display(head(x));

let y = stream_tail(x);
display(head(y));

y = tail(y);
display(head(y()));

y = stream_tail(y());
display(y);

x = stream_tail(x);
display(head(x));

x = stream_tail(x);
display(head(x));

x = stream_tail(x);
display(x);

x = list_to_stream(list([1], [2], [3]));
display(head(x));

y = stream_tail(x);
display(head(y));

y = tail(y);
display(head(y()));

y = stream_tail(y());
display(y);

x = stream_tail(x);
display(head(x));

x = stream_tail(x);
display(head(x));

x = stream_tail(x);
display(x);
