// test-ftrace.c
int factorial(int n) {
    if (n <= 1) return 1;
    else return n * factorial(n-1);
  }
  
  int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n-1) + fibonacci(n-2);
  }
  
  int main() {
    int a = factorial(5);
    int b = fibonacci(6);
    return a + b;
  }