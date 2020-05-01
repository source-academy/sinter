function ds(xs) {
    let new_list = [0, null];
    let last_pair = new_list;
    while (is_pair(xs)) {
        const p = [head(xs), null];
        last_pair[1] = p;
        last_pair = p;
        xs = tail(xs)();
    }
    display(new_list);
}

ds(stream_append(null, null));
ds(stream_append(null, stream(1, 2, 3, 4, 5)));
ds(stream_append(stream(1, 2, 3, 4, 5), null));
ds(stream_append(stream(1, 2, 3, 4, 5), stream(1, 2, 3, 4, 5)));
ds(stream_append([[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]],
    stream(1, 2, 3, 4, 5)));

ds(stream_append([[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]],
    [[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]]));
