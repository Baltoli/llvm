#include "llvm/Constraints/CustomPasses.hpp"
#include "llvm/Constraints/IdiomSpecifications.hpp"
#include "llvm/Constraints/Transforms.hpp"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include <fstream>
#include <string>
#include <vector>
#include <map>

using namespace llvm;

class ResearchPreprocessor : public ModulePass
{
public:
    static char ID;

    ResearchPreprocessor() : ModulePass(ID)
    {
        constraint_specs.emplace_back("HOISTSELECT",  DetectHoistSelect,  &transform_hoistselect_pattern);
        constraint_specs.emplace_back("DISTRIBUTIVE", DetectDistributive, &transform_distributive);
    }

    bool runOnModule(Module& module) override;

private:
    std::vector<std::tuple<std::string,std::vector<Solution>(*)(llvm::Function&,unsigned),
                                       void(*)(Function&,std::map<std::string,Value*>)>> constraint_specs;
};

bool ResearchPreprocessor::runOnModule(Module& module)
{
    ModuleSlotTracker slot_tracker(&module);

    std::ofstream ofs("preprocess-report.txt");

    for(Function& function : module.getFunctionList())
    {
        if(!function.isDeclaration())
        {
            bool found_something = false;

            for(const auto& spec : constraint_specs)
            {
                auto solutions = std::get<1>(spec)(function, UINT_MAX);

                if(!solutions.empty())
                {
                    if(!found_something)
                    {
                        ofs<<"BEGIN FUNCTION PREPROCESSING "<<(std::string)function.getName()<<"\n";
                        found_something = true;
                    }

                    // This is NOT safe!
                    // Solutions might overlap and then this is entirely unsafe, resulting in invalid pointers!
                    // Need to fix this.
                    for(auto& solution : solutions)
                    {
                        ofs<<"BEGIN "<<std::get<0>(spec)<<"\n"
                           <<solution.prune().print_json(slot_tracker)<<"\n"
                           <<"END "<<std::get<0>(spec)<<"\n";

                        (*std::get<2>(spec))(function, solution);
                    }
                }
            }

            if(found_something)
            {
                ofs<<"END FUNCTION PREPROCESSING\n";
            }
        }
    }

    ofs.close();
    return false;
}

char ResearchPreprocessor::ID = 0;

static RegisterPass<ResearchPreprocessor> X("research-preprocessor", "Research preprocessor", false, false);

ModulePass *llvm::createResearchPreprocessorPass() {
  return new ResearchPreprocessor();
}
