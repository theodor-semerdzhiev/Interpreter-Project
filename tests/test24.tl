class Triangle(a, b, c)
{
    let a = a;
    let b = b;
    let c = c;

    private class Edge(a, b)
    {
        let a = a;
        let b = b;

        let x = (a[0] + b[0]) / 2;
        let y = (a[1] + b[1]) / 2;
        let z = (a[2] + b[2]) / 2;

        let midpoint = [x, y, z];
    }

    let e1 = Edge(a, b);
    let e2 = Edge(b, c);
    let e3 = Edge(c, a);   

    func subdivide(tri_list)
    {
        let nt1 = Triangle(a, e1->midpoint, e3->midpoint);
        let nt2 = Triangle(e1->midpoint, b, e2->midpoint);
        let nt3 = Triangle(e2->midpoint, c, e3->midpoint);
        let nt4 = Triangle(e1->midpoint, e2->midpoint, e3->midpoint);

        let tl = [nt1, nt2, nt3, nt4];

        return tl;
    }

    func to_string()
    {
        print("[", a, b, c, "]\n");
    }
}

func print_list(list)
{
    for (let i = 0; i < len(list); i = i + 1;)
    {
        list[i]->to_string();
    }
}

let xx = [0, 3, 5];
let yy = [1, 7, 14];
let zz = [50, 3, -5];

let tt = Triangle(xx, yy, zz);
let current = [tt];
let tri_list = [tt];


for (let i = 0; i < 2; i = i + 1;)
{
    let tmp = [];

    for (let j = 0; j < len(current); j = j + 1;)
    {
        tmp = tmp + current[j]->subdivide(current);
    }

    current = tmp;
    tri_list = tri_list + tmp;
}

print_list(tri_list);

let s = set { 1,2 ,3 ,4, 5 * 3};
let m = map {1:2 , 3:3 , 4:5 ,  5* 23: "sds"};

let _ = 20 & 0;
print(_);
