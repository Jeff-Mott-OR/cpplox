function fib(n) {
  if (n < 2) return n;
  return fib(n - 2) + fib(n - 1);
}

let start = new Date().getTime();
console.log(fib(15) === 610);
console.log(new Date().getTime() - start);
