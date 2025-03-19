exception lol;

# Should return Buffer Overflow Exception
func test(n) {
    return test(n + 1);

}

raise lol("Hello");

test(0);