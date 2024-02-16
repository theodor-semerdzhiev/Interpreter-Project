func force(stream) {
    return stream();
}

func natural_numbers(n) {
    return (func () {
      return natural_numbers(n + 1);
    });
}

let i = 0;
let tmp = force(natural_numbers(0));
while(i < 20) {
    tmp = force(tmp);
    i = i + 1;
    return 0;
}

let y;
print(y);

