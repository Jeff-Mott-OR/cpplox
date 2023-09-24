let a1 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa1";
let a2 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa2";
let a3 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa3";
let a4 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa4";
let a5 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa5";
let a6 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa6";
let a7 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa7";
let a8 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa8";

let i = 0;

let loopStart = new Date().getTime();

while (i < 100) {
  i = i + 1;

  a1; a1; a1; a2; a1; a3; a1; a4; a1; a5; a1; a6; a1; a7; a1; a8;
  a2; a1; a2; a2; a2; a3; a2; a4; a2; a5; a2; a6; a2; a7; a2; a8;
  a3; a1; a3; a2; a3; a3; a3; a4; a3; a5; a3; a6; a3; a7; a3; a8;
  a4; a1; a4; a2; a4; a3; a4; a4; a4; a5; a4; a6; a4; a7; a4; a8;
  a5; a1; a5; a2; a5; a3; a5; a4; a5; a5; a5; a6; a5; a7; a5; a8;
  a6; a1; a6; a2; a6; a3; a6; a4; a6; a5; a6; a6; a6; a7; a6; a8;
  a7; a1; a7; a2; a7; a3; a7; a4; a7; a5; a7; a6; a7; a7; a7; a8;
  a8; a1; a8; a2; a8; a3; a8; a4; a8; a5; a8; a6; a8; a7; a8; a8;

  a1; a1; a1; a2; a1; a3; a1; a4; a1; a5; a1; a6; a1; a7; a1; a8;
  a2; a1; a2; a2; a2; a3; a2; a4; a2; a5; a2; a6; a2; a7; a2; a8;
  a3; a1; a3; a2; a3; a3; a3; a4; a3; a5; a3; a6; a3; a7; a3; a8;
  a4; a1; a4; a2; a4; a3; a4; a4; a4; a5; a4; a6; a4; a7; a4; a8;
  a5; a1; a5; a2; a5; a3; a5; a4; a5; a5; a5; a6; a5; a7; a5; a8;
  a6; a1; a6; a2; a6; a3; a6; a4; a6; a5; a6; a6; a6; a7; a6; a8;
  a7; a1; a7; a2; a7; a3; a7; a4; a7; a5; a7; a6; a7; a7; a7; a8;
  a8; a1; a8; a2; a8; a3; a8; a4; a8; a5; a8; a6; a8; a7; a8; a8;

  a1; a1; a1; a2; a1; a3; a1; a4; a1; a5; a1; a6; a1; a7; a1; a8;
  a2; a1; a2; a2; a2; a3; a2; a4; a2; a5; a2; a6; a2; a7; a2; a8;
  a3; a1; a3; a2; a3; a3; a3; a4; a3; a5; a3; a6; a3; a7; a3; a8;
  a4; a1; a4; a2; a4; a3; a4; a4; a4; a5; a4; a6; a4; a7; a4; a8;
  a5; a1; a5; a2; a5; a3; a5; a4; a5; a5; a5; a6; a5; a7; a5; a8;
  a6; a1; a6; a2; a6; a3; a6; a4; a6; a5; a6; a6; a6; a7; a6; a8;
  a7; a1; a7; a2; a7; a3; a7; a4; a7; a5; a7; a6; a7; a7; a7; a8;
  a8; a1; a8; a2; a8; a3; a8; a4; a8; a5; a8; a6; a8; a7; a8; a8;
}

let loopTime = new Date().getTime() - loopStart;

let start = new Date().getTime();

i = 0;
while (i < 100) {
  i = i + 1;

  // 1 === 1; 1 === 2; 1 === nil; 1 === "str"; 1 === true;
  // nil === nil; nil === 1; nil === "str"; nil === true;
  // true === true; true === 1; true === false; true === "str"; true === nil;
  // "str" === "str"; "str" === "stru"; "str" === 1; "str" === nil; "str" === true;

  a1 === a1; a1 === a2; a1 === a3; a1 === a4; a1 === a5; a1 === a6; a1 === a7; a1 === a8;
  a2 === a1; a2 === a2; a2 === a3; a2 === a4; a2 === a5; a2 === a6; a2 === a7; a2 === a8;
  a3 === a1; a3 === a2; a3 === a3; a3 === a4; a3 === a5; a3 === a6; a3 === a7; a3 === a8;
  a4 === a1; a4 === a2; a4 === a3; a4 === a4; a4 === a5; a4 === a6; a4 === a7; a4 === a8;
  a5 === a1; a5 === a2; a5 === a3; a5 === a4; a5 === a5; a5 === a6; a5 === a7; a5 === a8;
  a6 === a1; a6 === a2; a6 === a3; a6 === a4; a6 === a5; a6 === a6; a6 === a7; a6 === a8;
  a7 === a1; a7 === a2; a7 === a3; a7 === a4; a7 === a5; a7 === a6; a7 === a7; a7 === a8;
  a8 === a1; a8 === a2; a8 === a3; a8 === a4; a8 === a5; a8 === a6; a8 === a7; a8 === a8;

  a1 === a1; a1 === a2; a1 === a3; a1 === a4; a1 === a5; a1 === a6; a1 === a7; a1 === a8;
  a2 === a1; a2 === a2; a2 === a3; a2 === a4; a2 === a5; a2 === a6; a2 === a7; a2 === a8;
  a3 === a1; a3 === a2; a3 === a3; a3 === a4; a3 === a5; a3 === a6; a3 === a7; a3 === a8;
  a4 === a1; a4 === a2; a4 === a3; a4 === a4; a4 === a5; a4 === a6; a4 === a7; a4 === a8;
  a5 === a1; a5 === a2; a5 === a3; a5 === a4; a5 === a5; a5 === a6; a5 === a7; a5 === a8;
  a6 === a1; a6 === a2; a6 === a3; a6 === a4; a6 === a5; a6 === a6; a6 === a7; a6 === a8;
  a7 === a1; a7 === a2; a7 === a3; a7 === a4; a7 === a5; a7 === a6; a7 === a7; a7 === a8;
  a8 === a1; a8 === a2; a8 === a3; a8 === a4; a8 === a5; a8 === a6; a8 === a7; a8 === a8;

  a1 === a1; a1 === a2; a1 === a3; a1 === a4; a1 === a5; a1 === a6; a1 === a7; a1 === a8;
  a2 === a1; a2 === a2; a2 === a3; a2 === a4; a2 === a5; a2 === a6; a2 === a7; a2 === a8;
  a3 === a1; a3 === a2; a3 === a3; a3 === a4; a3 === a5; a3 === a6; a3 === a7; a3 === a8;
  a4 === a1; a4 === a2; a4 === a3; a4 === a4; a4 === a5; a4 === a6; a4 === a7; a4 === a8;
  a5 === a1; a5 === a2; a5 === a3; a5 === a4; a5 === a5; a5 === a6; a5 === a7; a5 === a8;
  a6 === a1; a6 === a2; a6 === a3; a6 === a4; a6 === a5; a6 === a6; a6 === a7; a6 === a8;
  a7 === a1; a7 === a2; a7 === a3; a7 === a4; a7 === a5; a7 === a6; a7 === a7; a7 === a8;
  a8 === a1; a8 === a2; a8 === a3; a8 === a4; a8 === a5; a8 === a6; a8 === a7; a8 === a8;
}

let elapsed = new Date().getTime() - start;
console.log("loop");
console.log(loopTime);
console.log("elapsed");
console.log(elapsed);
console.log("equals");
console.log(elapsed - loopTime);
