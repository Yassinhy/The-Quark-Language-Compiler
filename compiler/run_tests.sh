#!/bin/bash

# ============================================
# Compiler Test Suite
# ============================================

COMPILER="./quark"
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
TEMP_FILES=()

# ============================================
# Helper Functions
# ============================================

print_header() {
    echo -e "\n${BLUE}=== $1 ===${NC}"
}

print_test() {
    echo -n "  Test $1: $2... "
}

cleanup() {
    for file in "${TEMP_FILES[@]}"; do
        [ -f "$file" ] && rm -f "$file"
    done
    [ -f "./output" ] && rm -f "./output"
    rm -f *.asm
}

run_test() {
    local test_num=$1
    local test_name=$2
    local code=$3
    local expected=$4

    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    print_test "$test_num" "$test_name"

    local temp_file=$(mktemp /tmp/test_XXXXXX.qk)
    echo "$code" > "$temp_file"
    TEMP_FILES+=("$temp_file")

    "$COMPILER" "$temp_file" x86_64 output >/dev/null 2>&1
    local compile_status=$?

    if [ $compile_status -ne 0 ]; then
        echo -e "${RED}COMPILATION FAILED${NC}"
        echo -e "${YELLOW}Code:\n$code${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi

    if [ -f "./output" ]; then
        ./output >/dev/null 2>&1
        local exit_code=$?

        if [ $exit_code -eq "$expected" ]; then
            echo -e "${GREEN}PASS (exit $exit_code)${NC}"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo -e "${RED}FAIL (expected $expected, got $exit_code)${NC}"
            echo -e "${YELLOW}Code:\n$code${NC}"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    else
        echo -e "${RED}NO OUTPUT FILE${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi

    rm -f ./output
}

trap cleanup EXIT

# ============================================
# Test Suite
# ============================================

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}        COMPILER TEST SUITE             ${NC}"
echo -e "${BLUE}========================================${NC}"

# ============================================
# Basic Variable Declaration & Arithmetic
# ============================================
print_header "Basic Variable Declaration & Arithmetic"

run_test "1.1" "Basic arithmetic" \
"fn main(void): int {
    let x :int = 5;
    let y :int = 3;
    let z :int = x + y * 2;
    return z;
}
 " \
11

run_test "1.2" "Long and int operations" \
"fn main(void): int {
    let a :long = 100;
    let b :int = 20;
    let c :int = a - b;
    return c;
}
 " \
80

run_test "1.3" "Expression precedence" \
"fn main(void): int {
    let result :int = (10 + 5) * 2 - 8 / 4;
    return result;
}
 " \
28

run_test "1.4" "Variable reassignment" \
"fn main(void): int {
    let x :int = 5;
    x = x + 3;
    x = x * 2;
    return x;
}
 " \
16

run_test "1.5" "Negative numbers" \
"fn main(void): int {
    let a :int = -10;
    let b :int = 5;
    let c :int = a + b;
    return c;
}
 " \
251

run_test "1.6" "Modulo operator" \
"fn main(void): int {
    let x :int = 17;
    let y :int = 5;
    let result :int = x % y;
    return result;
}
 " \
2

# ============================================
# Comparison Operators
# ============================================
print_header "Comparison Operators (==, !=, <, >, <=, >=)"

run_test "2.1" "Equal comparison true" \
"fn main(void): int {
    let x :int = 5;
    if (x == 5) { return 1; }
    return 0;
}
 " \
1

run_test "2.2" "Equal comparison false" \
"fn main(void): int {
    let x :int = 5;
    if (x == 3) { return 1; }
    return 0;
}
 " \
0

run_test "2.3" "Not equal true" \
"fn main(void): int {
    let x :int = 5;
    if (x != 3) { return 1; }
    return 0;
}
 " \
1

run_test "2.4" "Not equal false" \
"fn main(void): int {
    let x :int = 5;
    if (x != 5) { return 1; }
    return 0;
}
 " \
0

run_test "2.5" "Less than true" \
"fn main(void): int {
    let x :int = 3;
    if (x < 5) { return 1; }
    return 0;
}
 " \
1

run_test "2.6" "Less than false" \
"fn main(void): int {
    let x :int = 5;
    if (x < 3) { return 1; }
    return 0;
}
 " \
0

run_test "2.7" "Greater than true" \
"fn main(void): int {
    let x :int = 5;
    if (x > 3) { return 1; }
    return 0;
}
 " \
1

run_test "2.8" "Greater than false" \
"fn main(void): int {
    let x :int = 3;
    if (x > 5) { return 1; }
    return 0;
}
 " \
0

run_test "2.9" "Less than or equal (equal)" \
"fn main(void): int {
    let x :int = 5;
    if (x <= 5) { return 1; }
    return 0;
}
 " \
1

run_test "2.10" "Less than or equal (less)" \
"fn main(void): int {
    let x :int = 3;
    if (x <= 5) { return 1; }
    return 0;
}
 " \
1

run_test "2.11" "Less than or equal false" \
"fn main(void): int {
    let x :int = 5;
    if (x <= 3) { return 1; }
    return 0;
}
 " \
0

run_test "2.12" "Greater than or equal (equal)" \
"fn main(void): int {
    let x :int = 5;
    if (x >= 5) { return 1; }
    return 0;
}
 " \
1

run_test "2.13" "Greater than or equal (greater)" \
"fn main(void): int {
    let x :int = 7;
    if (x >= 5) { return 1; }
    return 0;
}
 " \
1

run_test "2.14" "Greater than or equal false" \
"fn main(void): int {
    let x :int = 3;
    if (x >= 5) { return 1; }
    return 0;
}
 " \
0

# ============================================
# Control Flow: If/Else
# ============================================
print_header "Control Flow: If/Else"

run_test "3.1" "Simple if true" \
"fn main(void): int {
    let x :int = 10;
    if (x > 5) { return 1; }
    return 0;
}
 " \
1

run_test "3.2" "Simple if false" \
"fn main(void): int {
    let x :int = 3;
    if (x > 5) { return 1; }
    return 0;
}
 " \
0

run_test "3.3" "If-else chain" \
"fn main(void): int {
    let score :int = 85;
    let grade :int = 0;
    if (score >= 90) {
        grade = 1;
    } else if (score >= 80) {
        grade = 2;
    } else if (score >= 70) {
        grade = 3;
    } else {
        grade = 4;
    }
    return grade;
}
 " \
2

run_test "3.4" "Nested if" \
"fn main(void): int {
    let a :int = 10;
    let b :int = 20;
    if (a > 5) {
        if (b > 15) { return 1; }
    }
    return 0;
}
 " \
1

run_test "3.5" "Else only" \
"fn main(void): int {
    let x :int = 3;
    if (x > 5) {
        return 1;
    } else {
        return 2;
    }
}
 " \
2

# ============================================
# Loops
# ============================================
print_header "Loops"

run_test "4.1" "Simple while loop" \
"fn main(void): int {
    let counter :int = 0;
    while (counter < 5) {
        counter = counter + 1;
    }
    return counter;
}
 " \
5

run_test "4.2" "Nested loops" \
"fn main(void): int {
    let i :int = 0;
    let total :int = 0;
    while (i < 3) {
        let j :int = 0;
        while (j < 2) {
            total = total + 1;
            j = j + 1;
        }
        i = i + 1;
    }
    return total;
}
 " \
6

run_test "4.3" "Loop with break" \
"fn main(void): int {
    let x :int = 0;
    while (x < 10) {
        x = x + 1;
        if (x == 5) { break; }
    }
    return x;
}
 " \
5

run_test "4.4" "While loop countdown" \
"fn main(void): int {
    let x :int = 5;
    while (x > 0) { x = x - 1; }
    return x;
}
 " \
0

run_test "4.5" "Loop condition false from start" \
"fn main(void): int {
    let x :int = 0;
    while (x < 0) { x = 1; }
    return x;
}
 " \
0

run_test "4.6" "Loop summing values" \
"fn main(void): int {
    let sum :int = 0;
    let i :int = 0;
    while (i < 5) {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}
 " \
10

# ============================================
# Functions
# ============================================
print_header "Functions"

run_test "5.1" "Simple function call" \
"fn add(x: int, y: int): int {
    return x + y;
}
fn main(void): int {
    let result :int = add(3, 4);
    return result;
}
 " \
7

run_test "5.2" "Multiple functions" \
"fn max(a: int, b: int): int {
    if (a > b) { return a; }
    return b;
}
fn min(a: int, b: int): int {
    if (a < b) { return a; }
    return b;
}
fn main(void): int {
    let a :int = 10;
    let b :int = 20;
    return max(a, b) - min(a, b);
}
 " \
10

run_test "5.3" "Function with loop (factorial)" \
"fn factorial(n: int): int {
    let result :int = 1;
    let i :int = 1;
    while (i <= n) {
        result = result * i;
        i = i + 1;
    }
    return result;
}
fn main(void): int {
    return factorial(5);
}
 " \
120

run_test "5.4" "Chained function calls" \
"fn multiply(a: int, b: int): int {
    return a * b;
}
fn add_and_multiply(x: int, y: int, z: int): int {
    let sum :int = x + y;
    return multiply(sum, z);
}
fn main(void): int {
    return add_and_multiply(2, 3, 4);
}
 " \
20

run_test "5.5" "Function with conditional logic" \
"fn isEven(n: int): int {
    if (n % 2 == 0) { return 1; }
    return 0;
}
fn main(void): int {
    return isEven(4);
}
 " \
1

run_test "5.6" "Function returns absolute value" \
"fn absolute(x: int): int {
    if (x >= 0) { return x; }
    else { return -x; }
}
fn main(void): int {
    let neg :int = absolute(-10);
    let pos :int = absolute(5);
    return neg + pos;
}
 " \
15

run_test "5.7" "Function with no parameters" \
"fn getConstant(void): int {
    return 42;
}
fn main(void): int {
    return getConstant();
}
 " \
42

# ============================================
# Mixed Features
# ============================================
print_header "Mixed Features"

run_test "6.1" "Functions in conditions" \
"fn isEven(n: int): int {
    if (n % 2 == 0) { return 1; }
    return 0;
}
fn main(void): int {
    let count :int = 0;
    let i :int = 0;
    while (i < 10) {
        if (isEven(i) == 1) { count = count + 1; }
        i = i + 1;
    }
    return count;
}
 " \
5

run_test "6.2" "Fibonacci iterative" \
"fn fibonacci(n: int): int {
    if (n <= 1) { return n; }
    let a :int = 0;
    let b :int = 1;
    let i :int = 2;
    let result :int = 0;
    while (i <= n) {
        result = a + b;
        a = b;
        b = result;
        i = i + 1;
    }
    return result;
}
fn main(void): int {
    return fibonacci(10);
}
 " \
55

run_test "6.3" "Nested function calls" \
"fn square(x: int): int { return x * x; }
fn cube(x: int): int { return x * x * x; }
fn compute(x: int): int {
    if (x > 0) { return square(x) + cube(x); }
    else { return square(x) - cube(x); }
}
fn main(void): int {
    return compute(3);
}
 " \
36

# ============================================
# Edge Cases
# ============================================
print_header "Edge Cases"

run_test "7.1" "Zero handling in conditionals" \
"fn main(void): int {
    let x :int = 0;
    if (x == 0) { return 1; }
    return 0;
}
 " \
1

run_test "7.2" "Comparison chain with if-else" \
"fn main(void): int {
    let x :int = 7;
    let result :int = 0;
    if (x < 5) {
        result = 1;
    } else if (x < 10) {
        result = 2;
    } else {
        result = 3;
    }
    return result;
}
 " \
2

run_test "7.3" "Function with multiple return paths" \
"fn classify(n: int): int {
    if (n < 0) { return 1; }
    if (n == 0) { return 2; }
    if (n > 0) { return 3; }
    return 0;
}
fn main(void): int {
    return classify(5);
}
 " \
3

run_test "7.4" "Loop sum of even indices" \
"fn main(void): int {
    let sum :int = 0;
    let i :int = 0;
    while (i < 10) {
        if (i % 2 == 0) { sum = sum + i; }
        i = i + 1;
    }
    return sum;
}
 " \
20

run_test "7.5" "Deeply nested conditionals" \
"fn main(void): int {
    let a :int = 1;
    let b :int = 2;
    let c :int = 3;
    if (a < b) {
        if (b < c) {
            if (a < c) { return 1; }
        }
    }
    return 0;
}
 " \
1

# ============================================
# Complex Scenarios
# ============================================
print_header "Complex Scenarios"

run_test "8.1" "Complex expression chain" \
"fn add(x: int, y: int): int { return x + y; }
fn sub(x: int, y: int): int { return x - y; }
fn main(void): int {
    let a :int = 10;
    let b :int = 5;
    let c :int = 3;
    return add(a, b) * sub(a, c) / b;
}
 " \
21

run_test "8.2" "Multiple comparisons accumulate" \
"fn main(void): int {
    let x :int = 7;
    let result :int = 0;
    if (x > 0)  { result = result + 1; }
    if (x < 10) { result = result + 1; }
    if (x != 7) { result = result + 1; }
    return result;
}
 " \
2

run_test "8.3" "Function chaining" \
"fn inc(x: int): int { return x + 1; }
fn double(x: int): int { return x * 2; }
fn process(x: int): int { return double(inc(x)); }
fn main(void): int {
    return process(5);
}
 " \
12

run_test "8.4" "Loop with inner condition accumulate" \
"fn main(void): int {
    let x :int = 0;
    let result :int = 0;
    while (x < 5) {
        if (x > 1) { result = result + x; }
        x = x + 1;
    }
    return result;
}
 " \
9

run_test "8.5" "Complex control flow" \
"fn main(void): int {
    let x :int = 1;
    let y :int = 2;
    let z :int = 0;
    if (x == 1) {
        if (y == 2) { z = 10; }
        else { z = 20; }
    } else {
        z = 30;
    }
    return z;
}
 " \
10

# ============================================
# Recursion
# ============================================
print_header "Recursion"

run_test "9.1" "Recursive factorial" \
"fn factorial(n: int): int {
    if (n <= 1) { return 1; }
    return n * factorial(n - 1);
}
fn main(void): int {
    return factorial(6);
}
 " \
208

run_test "9.2" "Recursive fibonacci" \
"fn fib(n: int): int {
    if (n <= 1) { return n; }
    return fib(n - 1) + fib(n - 2);
}
fn main(void): int {
    return fib(8);
}
 " \
21

run_test "9.3" "Recursive sum 1..n" \
"fn sum(n: int): int {
    if (n <= 0) { return 0; }
    return n + sum(n - 1);
}
fn main(void): int {
    return sum(10);
}
 " \
55

run_test "9.4" "Recursive GCD" \
"fn gcd(a: int, b: int): int {
    if (b == 0) { return a; }
    return gcd(b, a % b);
}
fn main(void): int {
    return gcd(48, 18);
}
 " \
6

run_test "9.5" "Recursive power" \
"fn power(base: int, exp: int): int {
    if (exp == 0) { return 1; }
    return base * power(base, exp - 1);
}
fn main(void): int {
    return power(3, 4);
}
 " \
81

run_test "9.6" "Recursive count to target" \
"fn count_to(current: int, target: int): int {
    if (current == target) { return current; }
    return count_to(current + 1, target);
}
fn main(void): int {
    return count_to(0, 25);
}
 " \
25

run_test "9.7" "Recursive countdown" \
"fn countdown(n: int): int {
    if (n == 0) { return 0; }
    return countdown(n - 1);
}
fn main(void): int {
    return countdown(20);
}
 " \
0

run_test "9.8" "Recursive digit sum" \
"fn digit_sum(n: int): int {
    if (n < 10) { return n; }
    return (n % 10) + digit_sum(n / 10);
}
fn main(void): int {
    return digit_sum(493);
}
 " \
16

run_test "9.9" "Recursive max3" \
"fn max(a: int, b: int): int {
    if (a >= b) { return a; }
    return b;
}
fn max3(a: int, b: int, c: int): int {
    return max(a, max(b, c));
}
fn main(void): int {
    return max3(7, 15, 11);
}
 " \
15

run_test "9.10" "Recursive tail-like accumulator" \
"fn accumulate(n: int, acc: int): int {
    if (n == 0) { return acc; }
    return accumulate(n - 1, acc + n);
}
fn main(void): int {
    return accumulate(15, 0);
}
 " \
120

# ============================================
# Pointers
# ============================================
print_header "Pointers"

run_test "10.1" "Address-of basic" \
"fn main(void): int {
    let y :int = 42;
    let x :*int = &y;
    return 42;
}
 " \
42

run_test "10.2" "Two pointers to same variable" \
"fn main(void): int {
    let val :int = 99;
    let p1 :*int = &val;
    let p2 :*int = &val;
    return 99;
}
 " \
99

# ============================================
# Stress & Boundary
# ============================================
print_header "Stress & Boundary"

run_test "11.1" "Large loop iteration" \
"fn main(void): int {
    let i :int = 0;
    while (i < 200) { i = i + 1; }
    return 200;
}
 " \
200

run_test "11.2" "Zero arithmetic" \
"fn main(void): int {
    let x :int = 127;
    let y :int = x - 127;
    return y;
}
 " \
0

run_test "11.3" "Single iteration loop" \
"fn main(void): int {
    let x :int = 0;
    let count :int = 0;
    while (x < 1) {
        count = count + 1;
        x = x + 1;
    }
    return count;
}
 " \
1

run_test "11.4" "Chained functions 5 deep" \
"fn f1(x: int): int { return x + 1; }
fn f2(x: int): int { return f1(x) + 1; }
fn f3(x: int): int { return f2(x) + 1; }
fn f4(x: int): int { return f3(x) + 1; }
fn f5(x: int): int { return f4(x) + 1; }
fn main(void): int {
    return f5(0);
}
 " \
5

run_test "11.5" "Alternating add/sub in loop" \
"fn main(void): int {
    let total :int = 0;
    let i :int = 0;
    while (i < 10) {
        if (i % 2 == 0) { total = total + i; }
        else { total = total - i; }
        i = i + 1;
    }
    return total;
}
 " \
251

run_test "11.6" "Fibonacci fib(0) boundary" \
"fn fib(n: int): int {
    if (n <= 1) { return n; }
    return fib(n - 1) + fib(n - 2);
}
fn main(void): int {
    return fib(0);
}
 " \
0

run_test "11.7" "Factorial(0) == 1" \
"fn factorial(n: int): int {
    if (n <= 1) { return 1; }
    return n * factorial(n - 1);
}
fn main(void): int {
    return factorial(0);
}
 " \
1

run_test "11.8" "Nested loops with functions" \
"fn square(x: int): int { return x * x; }
fn main(void): int {
    let sum :int = 0;
    let i :int = 1;
    while (i <= 3) {
        let j :int = 1;
        while (j <= 3) {
            sum = sum + square(i + j);
            j = j + 1;
        }
        i = i + 1;
    }
    return sum;
}
 " \
156

run_test "11.9" "GCD of same numbers" \
"fn gcd(a: int, b: int): int {
    if (b == 0) { return a; }
    return gcd(b, a % b);
}
fn main(void): int {
    return gcd(7, 7);
}
 " \
7

run_test "11.10" "Power of 1 is always 1" \
"fn power(base: int, exp: int): int {
    if (exp == 0) { return 1; }
    return base * power(base, exp - 1);
}
fn main(void): int {
    return power(1, 100);
}
 " \
1

# ============================================
# Pointers (Extended)
# ============================================
print_header "Pointers (Extended)"

run_test "12.1" "Dereference single pointer" \
"fn main(void): int {
    let x :int = 77;
    let p :*int = &x;
    return p.*;
}
 " \
77

run_test "12.2" "Dereference double pointer" \
"fn main(void): int {
    let x :int = 33;
    let p :*int = &x;
    let pp :**int = &p;
    return pp.*.*;
}
 " \
33

run_test "12.3" "Pointer to modified variable" \
"fn main(void): int {
    let x :int = 10;
    x = x + 5;
    let p :*int = &x;
    return p.*;
}
 " \
15

run_test "12.4" "Two pointers, read both" \
"fn main(void): int {
    let a :int = 20;
    let b :int = 22;
    let pa :*int = &a;
    let pb :*int = &b;
    return pa.* + pb.*;
}
 " \
42

run_test "12.5" "Pointer passed to function" \
"fn deref(p: *int): int {
    return p.*;
}
fn main(void): int {
    let x :int = 99;
    let p :*int = &x;
    return deref(p);
}
 " \
99

run_test "12.6" "Double pointer passed to function" \
"fn double_deref(pp: **int): int {
    return pp.*.*;
}
fn main(void): int {
    let x :int = 55;
    let p :*int = &x;
    let pp :**int = &p;
    return double_deref(pp);
}
 " \
55

run_test "12.7" "Dereference in arithmetic" \
"fn main(void): int {
    let x :int = 10;
    let y :int = 32;
    let px :*int = &x;
    let py :*int = &y;
    return px.* + py.*;
}
 " \
42

run_test "12.8" "Dereference in condition" \
"fn main(void): int {
    let x :int = 7;
    let p :*int = &x;
    if (p.* > 5) { return 1; }
    return 0;
}
 " \
1

run_test "12.9" "Dereference in while condition" \
"fn main(void): int {
    let x :int = 5;
    let p :*int = &x;
    let sum :int = 0;
    while (p.* > 0) {
        sum = sum + 1;
        x = x - 1;
    }
    return sum;
}
 " \
5

run_test "12.10" "Triple pointer chain" \
"fn main(void): int {
    let x :int = 42;
    let p :*int = &x;
    let pp :**int = &p;
    let ppp :***int = &pp;
    return ppp.*.*.*;
}
" \
42

# ============================================
# Summary
# ============================================
echo -e "\n${BLUE}========================================${NC}"
echo -e "${BLUE}            TEST SUMMARY               ${NC}"
echo -e "${BLUE}========================================${NC}"
echo -e "Total tests run: $TOTAL_TESTS"
echo -e "${GREEN}Passed: $PASSED_TESTS${NC}"
if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}Failed: $FAILED_TESTS${NC}"
else
    echo -e "${RED}Failed: $FAILED_TESTS${NC}"
fi

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "\n${GREEN}All tests passed!${NC}"
else
    echo -e "\n${RED}Some tests failed.${NC}"
    exit 1
fi