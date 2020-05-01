display(stream_to_list(null));

display(stream_to_list(stream(1, 2, 3, 4, 5)));

display(stream_to_list([[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]]));

null;
