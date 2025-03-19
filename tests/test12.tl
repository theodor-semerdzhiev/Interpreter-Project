

func forEach(l, f) {
    let i = 0;
    for(;i < 30;;) {
        println(i);
        i = i + 1;
    }
}

let sum = 0;
forEach([[1,2,3,4],[2,3,5],[3,3,5],[4,3,5],[5,3,5],[6,3,5],[7,3,5],[8]], func(n) {
    println(n);
});

println(sum);
