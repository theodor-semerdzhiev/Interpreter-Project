

let PI = 3.1415926;

func sin(x, ite) {
    let res = x;
    let term = x;
    let sign = -1;
    let i = 2;
    while(i <= ite) {
        term = term * ((x * x) / ((i + 1) * i));
        res = res + sign * term;
        sign = sign * -1.0;
        i = i + 2;
    }
    return res;
}

print(sin(1/2*PI, 40));
