this pass is used on llvm-13

you can build it like this:
```
export LLVM_DIR=<installation/dir/of/llvm/13>
mkdir build
cd build
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR <source/dir/llvm/CFGPass>
make
```

after these commends you will get a file named libCFGPass.so

then you can use clang to translate it to IR:
```
cd ../test
clang-13 -O0 -S -emit-llvm test.c -o test.ll
```

now we can translate the IR by CFGPass:
```
opt-13 -load-pass-plugin ../build/libCFGPass.so -passes=CFGPass -disable-output test.ll
```
look at the test direction, there are some .dot file, let translate ff3.dot to .png:
```
dot -Tpng ff3.dot -o ff3.png
```
you will get a picture named ff3.png
