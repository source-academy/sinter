stream_for_each(display, null);

stream_for_each(display, stream(1, 2, 3, 4, 5));

stream_for_each(display, [[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]]);

stream_for_each(x => display(x * x), null);

stream_for_each(x => display(x * x), stream(1, 2, 3, 4, 5));

stream_for_each(x => display([x[0] * x[0]]), [[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]]);

null;
