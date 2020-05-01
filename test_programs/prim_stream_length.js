display(stream_length(null));

display(stream_length(stream(1, 2, 3, 4, 5)));

display(stream_length([[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]]));

null;
