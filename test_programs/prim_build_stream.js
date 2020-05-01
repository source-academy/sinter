let x = build_stream(5, x => x);
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

x = build_stream(5, math_exp);
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

x = build_stream(5, x => [x]);
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
