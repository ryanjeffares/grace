function fib(n) {
  if (n < 2) {
    return n;
  }
  return fib(n - 2) + fib(n - 1);
}

console.log(fib(5));
console.log(fib(10));
console.log(fib(20));
console.log(fib(25));
console.log(fib(30));
