#include "llvm/IDL/CustomPasses.hpp"
#include "llvm/IDL/Solution.hpp"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include <sstream>
#include <fstream>
#include <string>
#include <vector>

using namespace llvm;

class ResearchReplacerBase : public ModulePass
{
public:
    using Pivot  = std::function<Value*(const Solution&)>;
    using Action = std::function<void(Function&, Solution)>;

    static char ID;

    ResearchReplacerBase(std::vector<std::tuple<std::string,Pivot,Action>> i) : ModulePass(ID), idioms(i) { }

    bool runOnModule(Module& module) override;

private:
    std::vector<std::tuple<std::string,Pivot,Action>> idioms;
};

bool ResearchReplacerBase::runOnModule(Module& module)
{
    ModuleSlotTracker slot_tracker(&module);

    std::string filename = module.getName();
    for(char& c : filename) if(c == '/') c = '_';

    std::stringstream sstr;
    sstr<<"replace-report-"<<filename<<".json";
    std::ofstream ofs(sstr.str().c_str());
    ofs<<"{ \"filename\": \""<<(std::string)module.getName()<<"\",\n  \"detected\": [";

    char first_hit1 = true;
    for(Function& function : module.getFunctionList())
    {
        if(!function.isDeclaration())
        {
            for(auto& idiom : idioms)
            {
                for(auto& solution : GenerateAnalysis(std::get<0>(idiom))(function, 99))
                {
                    int pivot_begin = -1;
                    if(const auto& pivot_lookup = std::get<1>(idiom))
                        if(auto pivot = dyn_cast_or_null<Instruction>(pivot_lookup(solution)))
                            if(auto& debugloc = pivot->getDebugLoc())
                                pivot_begin = debugloc.getLine();

                    ofs<<(first_hit1?"{\n":", {\n");
                    ofs<<"    \"function\": \""<<(std::string)function.getName()<<"\",\n";
                    if(pivot_begin > 0)
                        ofs<<"    \"line\": "<<pivot_begin<<",\n";
                    ofs<<"    \"type\": \""<<std::get<0>(idiom)<<"\",\n";
                    ofs<<"    \"solution\":\n     ";
                    for(char c : solution.prune().print_json(slot_tracker))
                    {
                        ofs.put(c);
                        if(c == '\n') ofs<<"     ";
                    }
                    ofs<<"\n  }";
                    first_hit1 = false;

//                    if(std::get<2>(idiom))
//                        std::get<2>(idiom)(function, solution);
                }
            }
        }
    }

    ofs<<"]\n}\n";
    return false;
}

char ResearchReplacerBase::ID = 0;

void replace_idiom(Function& function, Solution solution, std::string harness_name,
                   Value* insertion, std::vector<Value*> variables, std::vector<Value*> removeeffects)
{
    Instruction* insertion_point = dyn_cast_or_null<Instruction>(insertion);

    std::vector<Value*> variable_values;
    std::vector<Type*>  variable_types;
    for(auto value : variables)
    {
        if(value)
        {
            variable_values.push_back(value);
            variable_types.push_back(value->getType());
        }
    }

    FunctionType* func_type = FunctionType::get(Type::getVoidTy(function.getContext()), variable_types, false);
    Constant*          func = function.getParent()->getOrInsertFunction(harness_name, func_type);

    CallInst::Create(func, variable_values, "", insertion_point);

    for(auto& value : removeeffects)
    {
        if(auto inst = dyn_cast_or_null<Instruction>(value))
        {
            inst->removeFromParent();
        }
    }
}

class ResearchReplacer : public ResearchReplacerBase
{
public:
    ResearchReplacer() : ResearchReplacerBase({
    {"GEMM", [](const Solution& s)->Value*{ return s["precursor"]; },
    [](Function& function, Solution solution) {
        replace_idiom(function, solution, "gemm_harness", solution["precursor"],
                      {solution["loop"][0]["iter_end"],
                       solution["loop"][1]["iter_end"],
                       solution["loop"][2]["iter_end"],
                       solution["iter_begin_read"]["base_pointer"],
                       solution["idx_read"]["base_pointer"],
                       solution["iter_end"]}, {solution["output"]["store"]}); }},
    {"SPMV_CSR", [](const Solution& s)->Value*{ return s["precursor"]; },
    [](Function& function, Solution solution) {
        replace_idiom(function, solution, "spmv_csr_harness", solution["successor"],
                      {solution["output"]["base_pointer"],
                       solution["matrix_read"]["base_pointer"],
                       solution["vector_read"]["base_pointer"],
                       solution["iter_begin_read"]["base_pointer"],
                       solution["index_read"]["base_pointer"],
                       solution["iter_end"]}, {solution["output"]["store"]}); }},
    {"SPMV_JDS",  [](const Solution& s)->Value*{ return s["precursor"]; }, nullptr},
    {"VectorAdd", [](const Solution& s)->Value*{ return s["precursor"]; }, nullptr},
    {"VectorDot", [](const Solution& s)->Value*{ return s["precursor"]; }, nullptr}}) { }
};

static RegisterPass<ResearchReplacer> X("research-replacer", "Research replacer", false, false);

ModulePass *llvm::createResearchReplacerPass() {
  return new ResearchReplacer();
}
