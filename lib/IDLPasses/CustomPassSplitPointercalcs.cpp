#include "llvm/IDLPasses/CustomPasses.hpp"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

using namespace llvm;

class ResearchSplitPointercalcs : public FunctionPass
{
public:
    static char ID;

    ResearchSplitPointercalcs() : FunctionPass(ID) {}

    bool runOnFunction(Function& function) override;
};

bool ResearchSplitPointercalcs::runOnFunction(Function& function)
{
    // Split so that only first index can be dynamic
    std::vector<std::pair<BasicBlock::iterator,Value*>> replacement_table;

    for(BasicBlock& block : function.getBasicBlockList())
    {
        for(Instruction& instruction : block.getInstList())
        {
            if(auto gep_instr = dyn_cast<GetElementPtrInst>(&instruction))
            {
                std::vector<std::vector<Value*>> index_groups;

                index_groups.emplace_back(1, gep_instr->getOperand(1));

                auto zero_index = ConstantInt::get(Type::getInt64Ty(function.getParent()->getContext()), 0);

                for(unsigned i = 2; i < gep_instr->getNumOperands(); i++)
                {
                    if(dyn_cast<ConstantInt>(gep_instr->getOperand(i)))
                    {
                        index_groups.back().push_back(gep_instr->getOperand(i));
                    }
                    else
                    {
                        index_groups.back().push_back(zero_index);
                        index_groups.push_back(std::vector<Value*>(1, gep_instr->getOperand(i)));
                    }
                }

                if(index_groups.size() > 1)
                {
                    Value* last_gep = gep_instr->getOperand(0);

                    for(unsigned i = 0; i < index_groups.size(); i++)
                    {
                        if(auto pointer_type = dyn_cast<PointerType>(last_gep->getType()))
                        {
                            last_gep = GetElementPtrInst::Create(pointer_type->getElementType(),
                                                                last_gep, index_groups[i], "", &instruction);
                        }
                    }

                    replacement_table.emplace_back(&instruction, last_gep);
                }
            }
        }
    }

    for(auto& entry : replacement_table)
    {
        ReplaceInstWithValue(entry.first->getParent()->getInstList(), entry.first, entry.second);
    }

    return false;
}

char ResearchSplitPointercalcs::ID = 0;

static RegisterPass<ResearchSplitPointercalcs> X("research-split-pointercalcs", "Research split pointer calculations", false, false);

FunctionPass *llvm::createResearchSplitPointercalcsPass() {
  return new ResearchSplitPointercalcs();
}
