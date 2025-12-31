long fib(long n, long acc, long prev) {
  if (n < 1) return acc;
  else return fib(n - 1, prev + acc, acc);
}
int main(int argc, char **argv) {
  const long n = 50;
  return fib(n, 0, 1);
}
