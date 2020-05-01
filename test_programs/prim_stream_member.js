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

ds(stream_member(null, null));
ds(stream_member(1, stream(1, 2, 3, 4, 5)));
ds(stream_member(2, stream(1, 2, 3, 4, 5)));
ds(stream_member(6, stream(1, 2, 3, 4, 5)));
ds(stream_member([1], stream([1], [2], [3], [4], [5])));

const x = [1];
ds(stream_member(x, stream([1], [2], x, [3], [4], [5])));

ds(stream_member(x, [[1], () => [[2], () => [x, () => [[4], () => [[5], () => null]]]]]));
