class Tree {
  constructor(item, depth) {
    this.item = item;
    this.depth = depth;
    if (depth > 0) {
      let item2 = item + item;
      depth = depth - 1;
      this.left = new Tree(item2 - 1, depth);
      this.right = new Tree(item2, depth);
    } else {
      this.left = null;
      this.right = null;
    }
  }

  check() {
    if (this.left === null) {
      return this.item;
    }

    return this.item + this.left.check() - this.right.check();
  }
}

let minDepth = 1;
let maxDepth = 4;
let stretchDepth = maxDepth + 1;

let start = new Date().getTime();

console.log("stretch tree of depth:");
console.log(stretchDepth);
console.log("check:");
console.log(new Tree(0, stretchDepth).check());

let longLivedTree = new Tree(0, maxDepth);

// iterations = 2 ** maxDepth
let iterations = 1;
let d = 0;
while (d < maxDepth) {
  iterations = iterations * 2;
  d = d + 1;
}

let depth = minDepth;
while (depth < stretchDepth) {
  let check = 0;
  let i = 1;
  while (i <= iterations) {
    check = check + new Tree(i, depth).check() + new Tree(-i, depth).check();
    i = i + 1;
  }

  console.log("num trees:");
  console.log(iterations * 2);
  console.log("depth:");
  console.log(depth);
  console.log("check:");
  console.log(check);

  iterations = iterations / 4;
  depth = depth + 2;
}

console.log("long lived tree of depth:");
console.log(maxDepth);
console.log("check:");
console.log(longLivedTree.check());
console.log("elapsed:");
console.log(new Date().getTime() - start);
