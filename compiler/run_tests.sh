#!/bin/bash

# ============================================
# Compiler Test Suite (Refined)
# ============================================

COMPILER="./my_compiler"
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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
    # Remove all temporary files
    for file in "${TEMP_FILES[@]}"; do
        if [ -f "$file" ]; then
            rm -f "$file"
        fi
    done
    # Remove output file if it exists
    if [ -f "./output" ]; then
        rm -f "./output"
    fi
    # Remove any leftover .asm files
    rm -f *.asm
}

run_test() {
    local test_num=$1
    local test_name=$2
    local code=$3
    local expected=$4
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    print_test "$test_num" "$test_name"
    
    # Create temporary test file
    local temp_file=$(mktemp /tmp/test_XXXXXX.ph)
    echo "$code" > "$temp_file"
    TEMP_FILES+=("$temp_file")
    
    # Compile
    "$COMPILER" "$temp_file" x86_64 >/dev/null 2>&1
    local compile_status=$?
    
    if [ $compile_status -ne 0 ]; then
        echo -e "${RED}COMPILATION FAILED${NC}"
        echo -e "${YELLOW}Code:\n$code${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
    
    # Run
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
    
    # Clean up output file
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
"let x :int = 5;
let y :int = 3;
let z :int = x + y * 2;
exit z;" \
11

run_test "1.2" "Long and int operations" \
"let a :long = 100;
let b :int = 20;
let c :int = a - b;
exit c;" \
80

run_test "1.3" "Expression precedence" \
"let result :int = (10 + 5) * 2 - 8 / 4;
exit result;" \
28

run_test "1.4" "Variable reassignment" \
"let x :int = 5;
x = x + 3;
x = x * 2;
exit x;" \
16

run_test "1.5" "Negative numbers" \
"let a :int = -10;
let b :int = 5;
let c :int = a + b;
exit c;" \
251  # -5 wraps to 251 as unsigned byte

run_test "1.6" "Modulo operator" \
"let x :int = 17;
let y :int = 5;
let result :int = x % y;
exit result;" \
2

# ============================================
# Comparison Operators
# ============================================
print_header "Comparison Operators (==, !=, <, >, <=, >=)"

run_test "2.1" "Equal comparison true" \
"let x :int = 5;
if (x == 5) {
    exit 1;
}
exit 0;" \
1

run_test "2.2" "Equal comparison false" \
"let x :int = 5;
if (x == 3) {
    exit 1;
}
exit 0;" \
0

run_test "2.3" "Not equal true" \
"let x :int = 5;
if (x != 3) {
    exit 1;
}
exit 0;" \
1

run_test "2.4" "Not equal false" \
"let x :int = 5;
if (x != 5) {
    exit 1;
}
exit 0;" \
0

run_test "2.5" "Less than true" \
"let x :int = 3;
if (x < 5) {
    exit 1;
}
exit 0;" \
1

run_test "2.6" "Less than false" \
"let x :int = 5;
if (x < 3) {
    exit 1;
}
exit 0;" \
0

run_test "2.7" "Greater than true" \
"let x :int = 5;
if (x > 3) {
    exit 1;
}
exit 0;" \
1

run_test "2.8" "Greater than false" \
"let x :int = 3;
if (x > 5) {
    exit 1;
}
exit 0;" \
0

run_test "2.9" "Less than or equal true (equal)" \
"let x :int = 5;
if (x <= 5) {
    exit 1;
}
exit 0;" \
1

run_test "2.10" "Less than or equal true (less)" \
"let x :int = 3;
if (x <= 5) {
    exit 1;
}
exit 0;" \
1

run_test "2.11" "Less than or equal false" \
"let x :int = 5;
if (x <= 3) {
    exit 1;
}
exit 0;" \
0

run_test "2.12" "Greater than or equal true (equal)" \
"let x :int = 5;
if (x >= 5) {
    exit 1;
}
exit 0;" \
1

run_test "2.13" "Greater than or equal true (greater)" \
"let x :int = 7;
if (x >= 5) {
    exit 1;
}
exit 0;" \
1

run_test "2.14" "Greater than or equal false" \
"let x :int = 3;
if (x >= 5) {
    exit 1;
}
exit 0;" \
0

# ============================================
# Control Flow: If/Else with Single Conditions
# ============================================
print_header "Control Flow: If/Else"

run_test "3.1" "Simple if true" \
"let x :int = 10;
if (x > 5) {
    exit 1;
}
exit 0;" \
1

run_test "3.2" "Simple if false" \
"let x :int = 3;
if (x > 5) {
    exit 1;
}
exit 0;" \
0

run_test "3.3" "If-else chain" \
"let score :int = 85;
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
exit grade;" \
2

run_test "3.4" "Nested if" \
"let a :int = 10;
let b :int = 20;
if (a > 5) {
    if (b > 15) {
        exit 1;
    }
}
exit 0;" \
1

run_test "3.5" "Else only" \
"let x :int = 3;
if (x > 5) {
    exit 1;
} else {
    exit 2;
}" \
2

# ============================================
# Loops
# ============================================
print_header "Loops"

run_test "4.1" "Simple while loop" \
"let counter :int = 0;
while (counter < 5) {
    counter = counter + 1;
}
exit counter;" \
5

run_test "4.2" "Nested loops" \
"let i :int = 0;
let j :int = 0;
let total :int = 0;
while (i < 3) {
    j = 0;
    while (j < 2) {
        total = total + 1;
        j = j + 1;
    }
    i = i + 1;
}
exit total;" \
6

run_test "4.3" "Loop with break condition" \
"let x :int = 0;
while (x < 10) {
    x = x + 1;
    if (x == 5) {
        break;
    }
}
exit x;" \
5

run_test "4.4" "While loop with countdown" \
"let x :int = 5;
while (x > 0) {
    x = x - 1;
}
exit x;" \
0

run_test "4.5" "Empty while loop (condition false)" \
"let x :int = 0;
while (x < 0) {
    x = 1;
}
exit x;" \
0

run_test "4.6" "Loop summing values" \
"let sum :int = 0;
let i :int = 0;
while (i < 5) {
    sum = sum + i;
    i = i + 1;
}
exit sum;" \
10

# ============================================
# Functions
# ============================================
print_header "Functions"

run_test "5.1" "Simple function call" \
"fn add(x: int, y: int): int {
    return x + y;
}
let result :int = add(3, 4);
exit result;" \
7

run_test "5.2" "Multiple functions" \
"fn max(a: int, b: int): int {
    if (a > b) {
        return a;
    }
    return b;
}
fn min(a: int, b: int): int {
    if (a < b) {
        return a;
    }
    return b;
}
let a :int = 10;
let b :int = 20;
let c :int = max(a, b) - min(a, b);
exit c;" \
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
let fact5 :int = factorial(5);
exit fact5;" \
120

run_test "5.4" "Function with parameters" \
"fn multiply(a: int, b: int): int {
    return a * b;
}
fn add_and_multiply(x: int, y: int, z: int): int {
    let sum :int = x + y;
    return multiply(sum, z);
}
let result :int = add_and_multiply(2, 3, 4);
exit result;" \
20

run_test "5.5" "Function with conditional logic" \
"fn isEven(n: int): int {
    if (n % 2 == 0) {
        return 1;
    }
    return 0;
}
let result :int = isEven(4);
exit result;" \
1

run_test "5.6" "Function returns absolute value" \
"fn absolute(x: int): int {
    if (x >= 0) {
        return x;
    } else {
        return -x;
    }
}
let neg :int = absolute(-10);
let pos :int = absolute(5);
exit neg + pos;" \
15

run_test "5.7" "Function with no parameters" \
"fn getConstant(void): int {
    return 42;
}
let result :int = getConstant();
exit result;" \
42

# ============================================
# Mixed Features (Complex Tests)
# ============================================
print_header "Mixed Features"

run_test "6.1" "Global and local variables" \
"let global :int = 100;
fn testScope(x: int): int {
    let local :int = 20;
    return x + local;
}
let result :int = testScope(global);
exit result;" \
120

run_test "6.2" "Functions in conditions" \
"fn isEven(n: int): int {
    if (n % 2 == 0) {
        return 1;
    }
    return 0;
}
let count :int = 0;
let i :int = 0;
while (i < 10) {
    if (isEven(i) == 1) {
        count = count + 1;
    }
    i = i + 1;
}
exit count;" \
5

run_test "6.3" "Fibonacci iterative" \
"fn fibonacci(n: int): int {
    if (n <= 1) {
        return n;
    }
    
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
let fib10 :int = fibonacci(10);
exit fib10;" \
55

run_test "6.4" "Nested function calls" \
"fn square(x: int): int {
    return x * x;
}
fn cube(x: int): int {
    return x * x * x;
}
fn compute(x: int): int {
    if (x > 0) {
        return square(x) + cube(x);
    } else {
        return square(x) - cube(x);
    }
}
let result :int = compute(3);
exit result;" \
36

# ============================================
# Edge Cases
# ============================================
print_header "Edge Cases"

run_test "7.1" "Single statement if" \
"let x :int = 5;
if (x > 0) exit 1;
exit 0;" \
1

run_test "7.2" "Zero handling in conditionals" \
"let x :int = 0;
if (x == 0) {
    exit 1;
}
exit 0;" \
1

run_test "7.3" "Comparison chain with if-else" \
"let x :int = 7;
let result :int = 0;
if (x < 5) {
    result = 1;
} else if (x < 10) {
    result = 2;
} else {
    result = 3;
}
exit result;" \
2

run_test "7.4" "Function with multiple return paths" \
"fn classify(n: int): int {
    if (n < 0) {
        return 1;
    }
    if (n == 0) {
        return 2;
    }
    if (n > 0) {
        return 3;
    }
    return 0;
}
let result :int = classify(5);
exit result;" \
3

run_test "7.5" "Loop counter with different conditions" \
"let sum :int = 0;
let i :int = 0;
while (i < 10) {
    if (i % 2 == 0) {
        sum = sum + i;
    }
    i = i + 1;
}
exit sum;" \
20

run_test "7.6" "Deeply nested conditionals" \
"let a :int = 1;
let b :int = 2;
let c :int = 3;
if (a < b) {
    if (b < c) {
        if (a < c) {
            exit 1;
        }
    }
}
exit 0;" \
1

# ============================================
# Comments Testing
# ============================================
print_header "Comments"

run_test "8.1" "Inline comments" \
"let x :int = 5;  // Initialize x
let y :int = x * 2;  // Calculate y
exit y;" \
10

run_test "8.2" "Commented function" \
"fn multiply(a: int, b: int): int {
    return a * b;
}
let result :int = multiply(3, 4);
exit result;" \
12

run_test "8.3" "Comments with control flow" \
"let x :int = 10;  // Start value
if (x > 0) {  // Check positive
    x = x * 2;  // Double it
}
exit x;" \
20

# ============================================
# Complex Scenarios
# ============================================
print_header "Complex Scenarios"

run_test "9.1" "Complex expression chain" \
"fn add(x: int, y: int): int {
    return x + y;
}
fn sub(x: int, y: int): int {
    return x - y;
}
let a :int = 10;
let b :int = 5;
let c :int = 3;
let result :int = add(a, b) * sub(a, c) / b;
exit result;" \
21

run_test "9.2" "Multiple comparison chains" \
"let x :int = 7;
let result :int = 0;
if (x > 0) {
    result = result + 1;
}
if (x < 10) {
    result = result + 1;
}
if (x != 7) {
    result = result + 1;
}
exit result;" \
2

run_test "9.3" "Function chaining" \
"fn inc(x: int): int {
    return x + 1;
}
fn double(x: int): int {
    return x * 2;
}
fn process(x: int): int {
    return double(inc(x));
}
let result :int = process(5);
exit result;" \
12

run_test "9.4" "Loop with multiple conditions" \
"let x :int = 0;
let result :int = 0;
while (x < 5) {
    if (x > 1) {
        result = result + x;
    }
    x = x + 1;
}
exit result;" \
9

run_test "9.5" "Recursive-like pattern with loop" \
"fn power(base: int, exp: int): int {
    let result :int = 1;
    let i :int = 0;
    while (i < exp) {
        result = result * base;
        i = i + 1;
    }
    return result;
}
let result :int = power(2, 5);
exit result;" \
32

run_test "9.6" "Complex control flow" \
"let x :int = 1;
let y :int = 2;
let z :int = 0;
if (x == 1) {
    if (y == 2) {
        z = 10;
    } else {
        z = 20;
    }
} else {
    z = 30;
}
exit z;" \
10

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
    echo -e "\n${GREEN}🎉 ALL TESTS PASSED! 🎉${NC}"
else
    echo -e "\n${RED}❌ SOME TESTS FAILED ❌${NC}"
    exit 1
fi