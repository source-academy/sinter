display(enum_stream(5, 4));

let x = enum_stream(6, 10);
display(head(x));

x = stream_tail(x);
display(head(x));

x = tail(x);
display(head(x()));

x = stream_tail(x());
display(head(x));

x = stream_tail(x);
display(head(x));

x = stream_tail(x);
display(x);

x = enum_stream(6.5, 10);
display(head(x));

x = stream_tail(x);
display(head(x));

x = tail(x);
display(head(x()));

x = stream_tail(x());
display(head(x));

x = stream_tail(x);
display(x);

x = enum_stream(6.5, 10.25);
display(head(x));

x = stream_tail(x);
display(head(x));

x = tail(x);
display(head(x()));

x = stream_tail(x());
display(head(x));

x = stream_tail(x);
display(x);
