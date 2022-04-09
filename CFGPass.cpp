#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/User.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Pass.h>
#include <fstream>
#include <llvm/Analysis/CFG.h>
#include <stdio.h>
#include <map>
using namespace llvm;

namespace {

    
    bool WriteFile(Function &F) {
            static char ID;
            std::error_code error;
            std::string str;
            //StringMap<int> basicblockMap;
            std::map<BasicBlock*, int> basicBlockMap;
            int bbCount;  //Block的编号
            bbCount = 0;

            
			raw_string_ostream rso(str);
			StringRef name(F.getName().str() + ".dot");
			
			enum sys::fs::OpenFlags F_None;
			raw_fd_ostream file(name, error, F_None);
			//std::ofstream os;
			//os.open(name.str() + ".dot");
			//if (!os.is_open()){
			//	errs() << "Could not open the " << name << "file\n";
			//	return false;
			//}
			file << "digraph \"CFG for'" + F.getName() + "\' function\" {\n";
			for (Function::iterator B_iter = F.begin(); B_iter != F.end(); ++B_iter){
				BasicBlock* curBB = &*B_iter;
				std::string name = curBB->getName().str();
				int fromCountNum;
				int toCountNum;
				if (basicBlockMap.find(curBB) != basicBlockMap.end())
				{
					fromCountNum = basicBlockMap[curBB];
				}
				else
				{
					fromCountNum = bbCount;
					basicBlockMap[curBB] = bbCount++;
				}

				file << "\tBB" << fromCountNum << " [shape=record, label=\"{";
				file << "BB" << fromCountNum << ":\\l\\l";
				for (BasicBlock::iterator I_iter = curBB->begin(); I_iter != curBB->end(); ++I_iter) {
					//printInstruction(&*I_iter, os);
					file << *I_iter << "\\l\n";
				}
				file << "}\"];\n";
				for (BasicBlock *SuccBB : successors(curBB)){
					if (basicBlockMap.find(SuccBB) != basicBlockMap.end())
					{
						toCountNum = basicBlockMap[SuccBB];
					}
					else
					{
						toCountNum = bbCount;
						basicBlockMap[SuccBB] = bbCount++;
					}

					file << "\tBB" << fromCountNum<< "-> BB"
						<< toCountNum << ";\n";
				}
			}
			file << "}\n";
			file.close();
			return false;
    }

    struct CFGPass : PassInfoMixin<CFGPass> {
        // Main entry point, takes IR unit to run the pass on (&F) and the
        // corresponding pass manager (to be queried if need be)
        PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
            WriteFile(F);
            return PreservedAnalyses::all();
        }

        // Without isRequired returning true, this pass will be skipped for functions
        // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
        // all functions with optnone.
        static bool isRequired() { return true; }
    };

    llvm::PassPluginLibraryInfo getHelloWorldPluginInfo() {
        return {LLVM_PLUGIN_API_VERSION, "CFGPass", LLVM_VERSION_STRING,
                [](PassBuilder &PB) {
                    PB.registerPipelineParsingCallback(
                        [](StringRef Name, FunctionPassManager &FPM,
                        ArrayRef<PassBuilder::PipelineElement>) {
                        if (Name == "CFGPass") {
                            FPM.addPass(CFGPass());
                            return true;
                        }
                        return false;
                        });
                }};
    }

    // This is the core interface for pass plugins. It guarantees that 'opt' will
    // be able to recognize HelloWorld when added to the pass pipeline on the
    // command line, i.e. via '-passes=hello-world'
    extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
    llvmGetPassPluginInfo() {
    return getHelloWorldPluginInfo();
    }

};
