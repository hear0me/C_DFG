#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/Pass.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Use.h>
#include <llvm/Analysis/CFG.h>
#include <list>
#include <llvm/IR/Dominators.h>
#include <llvm/ADT/DepthFirstIterator.h>
using namespace llvm;
namespace
{

	struct DFGPass : PassInfoMixin<DFGPass>
	{
	public:
		// the basic node pair
		typedef std::pair<Value *, StringRef> node;
		// the basic edge pair
		typedef std::pair<node, node> edge;
		// a list of node
		typedef std::list<node> node_list;
		// a list of edge
		typedef std::list<edge> edge_list;
		static char ID;

		edge_list inst_edges; // storage the queue of instruction
		edge_list edges;	  // storage the flow edges
		edge_list def_edges;  // anti dependence
		edge_list loop_edges; // loop dependence
		node_list call_nodes; // call nodes
		node_list nodes;	  // normal nodes
		int num = 0;

		// int tes=0;

		// change Instruction to string
		std::string changeIns2Str(Instruction *ins)
		{
			std::string temp_str;
			raw_string_ostream os(temp_str);
			ins->print(os);
			return os.str();
		}

		// get value's name
		StringRef getValueName(Value *v)
		{
			std::string temp_result = "val";
			if (v->getName().empty())
			{
				temp_result += std::to_string(num);
				num++;
			}
			else
			{
				temp_result = v->getName().str();
			}
			StringRef result(temp_result);
			// errs() << result;
			return result;
		}
		// Main entry point, takes IR unit to run the pass on (&F) and the
		// corresponding pass manager (to be queried if need be)
		PreservedAnalyses run(Function &F, FunctionAnalysisManager &)
		{
			std::error_code error;
			enum sys::fs::OpenFlags F_None;
			StringRef fileName(F.getName().str() + ".dot");
			raw_fd_ostream file(fileName, error, F_None);

			errs() << "Hello\n";

			edges.clear();
			nodes.clear();
			call_nodes.clear();
			inst_edges.clear();
			def_edges.clear();
			loop_edges.clear();

			// extract loop dependence from every loop of the function
			DominatorTree Dt(F);
			LoopInfo LI(Dt);
			for (Loop *TopLevelLoop : LI)
			{
				for (Loop *L : depth_first(TopLevelLoop))
				{
					auto LoopFirstIns = L->getHeader()->getFirstNonPHI();
					for (Loop::block_iterator BB = L->block_begin(); BB != L->block_end(); ++BB)
					{
						BasicBlock *b = *BB;
						for (BasicBlock::iterator I = b->begin(), IEnd = b->end(); I != IEnd; ++I)
						{
							Instruction *curII = &*I;
							loop_edges.push_back(edge(node(LoopFirstIns, getValueName(LoopFirstIns)), node(curII, getValueName(curII))));
						}
					}
				}
			}

			// extract normal edge like flow, anti dependence
			for (Function::iterator BB = F.begin(), BEnd = F.end(); BB != BEnd; ++BB)
			{
				BasicBlock *curBB = &*BB;
				for (BasicBlock::iterator II = curBB->begin(), IEnd = curBB->end(); II != IEnd; ++II)
				{
					Instruction *curII = &*II;
					// errs() << getValueName(curII) << "\n";
					switch (curII->getOpcode())
					{
					// load and store instruction special process
					case llvm::Instruction::Load:
					{
						LoadInst *linst = dyn_cast<LoadInst>(curII);
						Value *loadValPtr = linst->getPointerOperand();
						edges.push_back(edge(node(loadValPtr, getValueName(loadValPtr)), node(curII, getValueName(curII))));
						break;
					}
					case llvm::Instruction::Store:
					{
						StoreInst *sinst = dyn_cast<StoreInst>(curII);
						Value *storeValPtr = sinst->getPointerOperand();
						Value *storeVal = sinst->getValueOperand();
						edges.push_back(edge(node(storeVal, getValueName(storeVal)), node(curII, getValueName(curII))));
						def_edges.push_back(edge(node(curII, getValueName(curII)), node(storeValPtr, getValueName(storeValPtr))));
						break;
					}
					default:
					{
						for (Instruction::op_iterator op = curII->op_begin(), opEnd = curII->op_end(); op != opEnd; ++op)
						{
							Instruction *tempIns;
							if (dyn_cast<Instruction>(*op))
							{
								edges.push_back(edge(node(op->get(), getValueName(op->get())), node(curII, getValueName(curII))));
							}
						}
						break;
					}
					}
					BasicBlock::iterator next = II;
					// errs() << curII << "\n";
					// call instruction special process
					auto *CI = dyn_cast<CallBase>(&*II);
					if (CI)
					{
						call_nodes.push_back(node(curII, getValueName(curII)));
						// errs() <<"call" << tes++ << curII->getOpcode()<<"size"<<call_nodes.size()<<"\n";
					}
					else
					{
						nodes.push_back(node(curII, getValueName(curII)));
					}
					++next;
					// extract inst_edge of basicblock
					if (next != IEnd)
					{
						inst_edges.push_back(edge(node(curII, getValueName(curII)), node(&*next, getValueName(&*next))));
					}
				}
				// extract connect edge between blocks
				Instruction *terminator = curBB->getTerminator();
				for (BasicBlock *sucBB : successors(curBB))
				{
					Instruction *first = &*(sucBB->begin());
					inst_edges.push_back(edge(node(terminator, getValueName(terminator)), node(first, getValueName(first))));
				}
			}
			// errs() << "Write\n";
			file << "digraph \"DFG for'" + F.getName() + "\' function\" {\n";
			// errs() << "Write DFG\n";

			// write to file
			for (node_list::iterator node = nodes.begin(), node_end = nodes.end(); node != node_end; ++node)
			{
				file << "\tNode" << node->first << "[shape=record, label=\"" << *(node->first) << "\"];\n";
			}

			for (node_list::iterator node = call_nodes.begin(), node_end = call_nodes.end(); node != node_end; ++node)
			{
				file << "\tNode" << node->first << "[shape=record, label=\"" << *(node->first) << "\", color=blue];\n";
			}

			for (edge_list::iterator edge = inst_edges.begin(), edge_end = inst_edges.end(); edge != edge_end; ++edge)
			{
				file << "\tNode" << edge->first.first << " -> Node" << edge->second.first << "[label=\"[control_flow]\"];"<< "\n";
			}

			// special edges
			file << "edge [color=red]"<< "\n";
			for (edge_list::iterator edge = loop_edges.begin(), edge_end = loop_edges.end(); edge != edge_end; ++edge)
			{
				file << "\tNode" << edge->first.first << " -> Node" << edge->second.first << "[label=\"[loop_dep]\"];"<< "\n";
			}
			for (edge_list::iterator edge = def_edges.begin(), edge_end = def_edges.end(); edge != edge_end; ++edge)
			{
				file << "\tNode" << edge->first.first << " -> Node" << edge->second.first << "[label=\"[anti_dep]\"];"<< "\n";
			}
			for (edge_list::iterator edge = edges.begin(), edge_end = edges.end(); edge != edge_end; ++edge)
			{
				file << "\tNode" << edge->first.first << " -> Node" << edge->second.first << "[label=\"[flow_dep]\"];"<< "\n";
			}
			errs() << "Write Done\n";
			file << "}\n";
			file.close();
			// return false;
			return PreservedAnalyses::all();
		}

		// Without isRequired returning true, this pass will be skipped for functions
		// decorated with the optnone LLVM attribute. Note that clang -O0 decorates
		// all functions with optnone.
		static bool isRequired() { return true; }
	};

};

//register pass
llvm::PassPluginLibraryInfo getDFGPassPluginInfo()
{
	return {LLVM_PLUGIN_API_VERSION, "DFGPass", LLVM_VERSION_STRING,
			[](PassBuilder &PB)
			{
				PB.registerPipelineParsingCallback(
					[](StringRef Name, FunctionPassManager &FPM,
					   ArrayRef<PassBuilder::PipelineElement>)
					{
						if (Name == "DFGPass")
						{
							FPM.addPass(DFGPass());
							return true;
						}
						return false;
					});
			}};
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize HelloWorld when added to the pass pipeline on the
// command line, i.e. via '-passes=DFGPass'

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo()
{
	return getDFGPassPluginInfo();
}
