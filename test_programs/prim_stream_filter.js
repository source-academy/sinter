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

ds(stream_filter(null, null));
ds(stream_filter(x => x % 2 === 0, null));
ds(stream_filter(x => x % 2 === 0, stream(1, 2, 3, 4, 5)));
ds(stream_filter(x => x[0] % 2 === 0, [[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]]));
ds(stream_filter(x => false, stream(1, 2, 3, 4, 5)));
ds(stream_filter(x => false, [[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]]));
