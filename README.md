# Nystrom's Crafting Interpreters: An Implementation in C++

[![Travis CI status](https://travis-ci.org/Jeff-Mott-OR/cpplox.svg?branch=master)](https://travis-ci.org/Jeff-Mott-OR/cpplox)

I decided to work my way through Nystrom's book [Crafting Interpreters](http://www.craftinginterpreters.com/), but instead of Java or C that Nystrom used and will use, I decided to write my implementation in C++.

If you, my only reader, try to compare my implementation to Nystrom's code samples, be aware that I didn't merely copy-paste Nystrom's code varbatim. I considered every line, and I often made changes. Sometimes the changes were mandatory and a consequence of writing in a different language. Sometimes the changes were optional but still a consequence of writing in a different language; each language has its own idea of good style. And sometimes the changes were optional and a reflection of my personal taste and opinions.

For me, this isn't just an exercise in interpreters; it's an exercise in C++ and its ecosystem of libraries, platforms, tools, and practices. Feel free to offer suggestions where I can do better.

## Build

    mkdir build
    cd build

    cmake ..
    cmake --build .

## Copyright

Copyright 2018 Jeff Mott. [MIT License](https://opensource.org/licenses/MIT).
