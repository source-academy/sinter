display(build_list(5, x => 0));
display(build_list(5, x => x));
display(build_list(5, x => x + 1));
display(build_list(5, x => [x]));
display(build_list(5, list));
display(map(is_number, build_list(5, math_random)));

null;
