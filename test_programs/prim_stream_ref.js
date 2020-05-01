display(stream_ref(stream(1, 2, 3, 4, 5), 0));
display(stream_ref(stream(1, 2, 3, 4, 5), 1));
display(stream_ref(stream(1, 2, 3, 4, 5), 2));
display(stream_ref(stream(1, 2, 3, 4, 5), 3));
display(stream_ref(stream(1, 2, 3, 4, 5), 4));

display(stream_ref([[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]], 0));
display(stream_ref([[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]], 1));
display(stream_ref([[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]], 2));
display(stream_ref([[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]], 3));
display(stream_ref([[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]], 4));
