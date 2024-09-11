# prints program arguments
println(__args__);

let number = 1.5;

func forEach(l, f) {
    for(let i = 0; i < len(l); i = i + 1;) {
        f(l[i]);
    }
}

forEach([1,2,3,4,5,6,7,8, 2.3412, 4.56, 14.23], func (n) { 
    println(str(n + 1 + number));
});