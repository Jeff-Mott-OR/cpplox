let i = 0;

let loopStart = new Date().getTime();

while (i < 1000) {
  i = i + 1;

  1; 1; 1; 2; 1; null; 1; "str"; 1; true;
  null; null; null; 1; null; "str"; null; true;
  true; true; true; 1; true; false; true; "str"; true; null;
  "str"; "str"; "str"; "stru"; "str"; 1; "str"; null; "str"; true;
}

let loopTime = new Date().getTime() - loopStart;

let start = new Date().getTime();

i = 0;
while (i < 1000) {
  i = i + 1;

  1 === 1; 1 === 2; 1 === null; 1 === "str"; 1 === true;
  null === null; null === 1; null === "str"; null === true;
  true === true; true === 1; true === false; true === "str"; true === null;
  "str" === "str"; "str" === "stru"; "str" === 1; "str" === null; "str" === true;
}

let elapsed = new Date().getTime() - start;
console.log("loop");
console.log(loopTime);
console.log("elapsed");
console.log(elapsed);
console.log("equals");
console.log(elapsed - loopTime);
