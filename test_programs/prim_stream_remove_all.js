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

ds(stream_remove_all(0, stream(1, 2, 3, 4, 5)));
ds(stream_remove_all(1, stream(1, 2, 3, 4, 5)));
ds(stream_remove_all(1, stream(1, 1, 2, 3, 4, 5)));
ds(stream_remove_all(4, stream(1, 2, 3, 4, 5)));
ds(stream_remove_all(4, stream(1, 2, 3, 4, 4, 5)));
ds(stream_remove_all(5, stream(1, 2, 3, 4, 5)));
ds(stream_remove_all(5, stream(1, 2, 3, 4, 5, 5)));

const x = [1];
ds(stream_remove_all([1], [[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]]));
ds(stream_remove_all(x, [[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]]));
ds(stream_remove_all(x, [x, () => [x, () => [[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]]]]));
ds(stream_remove_all(x, [[1], () => [x, () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]]]));
ds(stream_remove_all(x, [[1], () => [x, () => [x, () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]]]]));
