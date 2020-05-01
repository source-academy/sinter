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

ds(stream_remove(0, stream(1, 2, 3, 4, 5)));
ds(stream_remove(1, stream(1, 2, 3, 4, 5)));
ds(stream_remove(1, stream(1, 1, 2, 3, 4, 5)));
ds(stream_remove(4, stream(1, 2, 3, 4, 5)));
ds(stream_remove(4, stream(1, 2, 3, 4, 4, 5)));
ds(stream_remove(5, stream(1, 2, 3, 4, 5)));
ds(stream_remove(5, stream(1, 2, 3, 4, 5, 5)));

const x = [1];
ds(stream_remove([1], [[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]]));
ds(stream_remove(x, [[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]]));
ds(stream_remove(x, [x, () => [x, () => [[1], () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]]]]));
ds(stream_remove(x, [[1], () => [x, () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]]]));
ds(stream_remove(x, [[1], () => [x, () => [x, () => [[2], () => [[3], () => [[4], () => [[5], () => null]]]]]]]));
