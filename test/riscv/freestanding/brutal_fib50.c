long fib(long n, long acc, long prev) {
  if (n < 1) return acc;
  else return fib(n - 1, prev + acc, acc);
}
__attribute__((used, retain)) extern long my_start(long n) { return fib(n, 0, 1); }
int main() {}
