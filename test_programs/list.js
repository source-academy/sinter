function print_list(xs) {
  if (xs === null) {
    return null;
  } else {
    display(head(xs));
    return print_list(tail(xs));
  }
}

const xs = list(1, 2, 3, 4, 5);
display(xs);

print_list(xs);
