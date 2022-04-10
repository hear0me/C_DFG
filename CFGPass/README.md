this pass is used on llvm-13

you can build it like this:

export LLVM_DIR=<installation/dir/of/llvm/13>
mkdir build
cd build
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR <source/dir/llvm/CFGPass>
make

then you should have a test dir and write a .c programe:

cd ..
mkdir test
touch test.c
vim test.c
