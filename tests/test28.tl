

func power(n) {
    if(n == 0) {
        return func (x) {return 1;};
    } else {
        let r = power(n-1);
        return func (x) {
            return x * r(x);
        };
    }
}

let pow2 = power(4);
print((pow2(10)));
