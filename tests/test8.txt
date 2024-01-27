func test(n) {
    let x = 30;
    return func() {
        return x + n;
    };
}
let y = test(40);

func fib(n) {
    if(n == 0) {
        return 0;
    }
    if(n == 1) {
        return 1;
    }
    return fib(n-1) + fib(n-2);
}

print(fib(20), "\n");
