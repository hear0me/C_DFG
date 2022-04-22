# llvm_DFGPass
llvm Data Flow Graph Analysis
---

This is a dynamic LLVM Pass which is to give the basicblocks id and profile the ids.

### Build the pass by doing that:
- go to project folder by `cd yourdirectory/DFGPass`

- create a folder by `mkdir build`

- cd the build folder and build it: `cd build && cmake ../`

- make the project: `make`

### execute by the Pass
- go to test folder by`cd ../test`
- you can transform test.c to test.ll by`clang-13 -O0 -S -emit-llvm test.c -o test.ll`
- now we can analyse it by plungin `opt-13 -load-pass-plugin ~/桌面/C_DFG/CFG_DFG_generator/LLVM_Pass/llvm_DFGPass/build/libDFGPass.so -passes=DFGPass -disable-output test.ll`
- for every function of your test.c, you'll get a .dot file
- now you can change the dot to png by`dot -Tpng main.dot -o main.png`

then there are the outputs.
