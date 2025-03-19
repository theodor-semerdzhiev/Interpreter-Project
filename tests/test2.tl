

func fact(n) {
    if (n == 0) {
        return 1;
    } else {
        return n * fact(n-1);
    }
}

let x = str(fact(20));
print(typeof(x), x + " 123123\n");
print("hello" * 5);


