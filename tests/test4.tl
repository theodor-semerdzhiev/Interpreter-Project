
func unfold(function, n, iteration) {
    if(iteration == 0) {
        return n;
    } else {
        return unfold(function, function(n), iteration - 1);
    }
}

let n = unfold(
    func (n) {
        println(n);
        return (n+1 / n) * n;
    }, 1, 5);
    
print(n, "\n");
println(2);

let x = "2";
let y = "2";

if(x == "2" && y == "2") {
    println(x, "+", y, "=", "4");
}

