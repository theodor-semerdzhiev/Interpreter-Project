
class Tuple(list_) {
    private let tuple = func () {
        if(typeof(list_) == "List") {
            return list_;
        } else {
            println("Argument must be a list");
            return [];
        }
    }();
    
    let val = 10;
    
    func printTuple() {

        print("(");
        for(let i=0; i < len(tuple); i = i + 1;) {
            print(tuple[i]);
            if(i < (len(tuple) - 1)) {
                print(", ");
            }
        } 
        print(")");
    }

    func getIndex(i) {
        if((len(tuple) <= i) || 0 > i) {
            println(str(i) + " is out of range for tuple of length " + str(i));
            return;
        }

        return tuple[i];
    }

    func toString() {
        let s = "";

        for(let i=0; i < len(tuple); i = i + 1;) {
            s = s + str(tuple[i]);
            if(i < (len(tuple) - 1)) {
                s = s + ", ";
            }
        }

        return "(" + s + ")";
    }
}

let t = Tuple([1,3,4,23,1,2,3,4,5,6,7,8]);
let s = t->toString();
println(s);
println(t);
println(t->getIndex(0));
