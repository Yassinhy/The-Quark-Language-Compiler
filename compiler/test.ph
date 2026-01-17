fn add(x:int):int {
    
    let y:int = 67;
    if (x == y) {   ## if X is equal to Y
        exit 5;
    }
    else {          ## if X in not equal to Y
        exit y;
    }
}

let x:int = 8;
add(x);
x = 5 + 3;
exit x;