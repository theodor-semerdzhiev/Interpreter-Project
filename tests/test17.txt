func twoSum(nums, target) {
    let numMap = map {};
    
    for (let i = 0; i < len(nums); i=i+1;) {
        let complement = target - nums[i];
        if (numMap->containsKey(complement)) {
            return [numMap[complement], i];
        }
        numMap->add(nums[i], i);
    }
    
    return null; 
}

let nums = [1,2,3,4,5,6,7,8, -100, 23, 100];
let res = twoSum(nums, 4);

if(res == null) {
    println(res);
} else {
    println(nums[res[0]],nums[res[1]]);
}

println(10 * [0,2]);


let s1 = set {1, 2, 3, 4, 5, 6, 7};
let s2 = set {4, 5, 6, 7, 8, 9, 10};

let s3 = set {1 ,7,9,10,11,11.23};

func sortSet(s) {
    return s->toList()->sort();
}

println(sortSet(s1->union(s2)->union(s3)));


