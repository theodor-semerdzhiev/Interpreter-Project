

func f(g) {
    return g() + g();
}



let x= f(func () { return 0.5;});
println(x);