display(is_list(list(1, 2, 3, 4, 5)));
display(is_list([1, [2, [3, [4, [5, null]]]]]));
display(is_list(null));
display(is_list(list([1], [2], [3], [4], [5])));

display(is_list([1, [2, [3, [4, [5]]]]]));
display(is_list([1, [2, [3, [4, [5, null, 5]]]]]));
