
let PI = 3.1415926;

func sin(x) {
    let res = x;
    let term = x;
    let sign = -1;
    let i = 2;

    while(i < 20) {
        term = term * ((x * x) / ((i + 1) * i));
        res = res + sign * term;
        sign = sign * -1.0;
        i = i + 2;
    }
    return res;
}

func toRad(deg) {
    return (deg / 180) * PI;
}

print(sin(toRad(90)), "\n");
print(sin(toRad(60)), "\n");

let l = [];
l->append(10, 14);
println(l);



