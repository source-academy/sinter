function ds(xs) {
    let new_list = [0, null];
    let last_pair = new_list;
    while (is_pair(xs)) {
        const p = [head(xs), null];
        last_pair[1] = p;
        last_pair = p;
        xs = tail(xs)();
    }
    display(new_list[1]);
}

ds(stream_reverse(null));
ds(stream_reverse(stream(1)));
ds(stream_reverse(stream(1, 2, 3, 4, 5)));
ds(stream_reverse([[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]]));
