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

ds(stream_map(null, null));
ds(stream_map(x => x % 2 === 0, null));
ds(stream_map(math_exp, stream(1, 2, 3, 4, 5)));
ds(stream_map(x => x + 1, stream(1, 2, 3, 4, 5)));
ds(stream_map(x => [x[0] * x[0]], [[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]]));
