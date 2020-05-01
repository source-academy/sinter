display(is_stream(null));
display(is_stream(list(1, 2, 3)));
display(is_stream(stream(1, 2, 3)));
display(is_stream([[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]]));
