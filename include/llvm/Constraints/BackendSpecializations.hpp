#ifndef _BACKEND_SPECIALIZATIONS_HPP_
#define _BACKEND_SPECIALIZATIONS_HPP_
#include "llvm/Constraints/BackendClasses.hpp"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/Timer.h"
#include <vector>

template<unsigned ... constants>
class BackendConstantValue : public BackendSingle
{
public:
    BackendConstantValue() = default;
    BackendConstantValue(const FunctionWrap& wrap)
      : BackendSingle(std::vector<unsigned>{constants...}) { }
};

template<typename Type>
class BackendLLVMSingle : public BackendSingle
{
public:
    BackendLLVMSingle() = default;
    BackendLLVMSingle(const FunctionWrap& wrap, std::function<bool(Type&)> pred)
      : BackendSingle(compute_hits(wrap, pred)) { }

private:
    static std::vector<unsigned> compute_hits(const FunctionWrap& wrap, std::function<bool(Type&)> pred)
    {
        std::vector<unsigned> hits;

        for(unsigned i = 0; i < wrap.size(); i++)
        {
            if(auto value = llvm::dyn_cast<Type>(wrap[i]))
            {
                if(pred == nullptr || pred(*value))
                {
                    hits.push_back(i);
                }
            }
        }

        return hits;
    }
};

class BackendNotNumericConstant : public BackendLLVMSingle<llvm::Value>
{
public:
    BackendNotNumericConstant() = default;
    BackendNotNumericConstant(const FunctionWrap& wrap)
      : BackendLLVMSingle<llvm::Value>(wrap, [](llvm::Value& value)
                                              { return !llvm::isa<llvm::ConstantInt>(value) &&
                                                       !llvm::isa<llvm::ConstantFP>(value)  &&
                                                       !llvm::isa<llvm::Function>(value)  &&
                                                       !llvm::isa<llvm::ConstantPointerNull>(value); }) { }
};

class BackendConstant : public BackendLLVMSingle<llvm::Constant>
{
public:
    BackendConstant() = default;
    BackendConstant(const FunctionWrap& wrap)
      : BackendLLVMSingle<llvm::Constant>(wrap, nullptr) { }
};


class BackendPreexecution : public BackendLLVMSingle<llvm::Value>
{
public:
    BackendPreexecution() = default;
    BackendPreexecution(const FunctionWrap& wrap)
      : BackendLLVMSingle<llvm::Value>(wrap, [](llvm::Value& value)
                                             { return !llvm::isa<llvm::Instruction>(value); }) { }
};

class BackendArgument : public BackendLLVMSingle<llvm::Argument>
{
public:
    BackendArgument() = default;
    BackendArgument(const FunctionWrap& wrap)
      : BackendLLVMSingle<llvm::Argument>(wrap, nullptr) { }
};

class BackendInstruction : public BackendLLVMSingle<llvm::Instruction>
{
public:
    BackendInstruction() = default;
    BackendInstruction(const FunctionWrap& wrap)
      : BackendLLVMSingle<llvm::Instruction>(wrap, nullptr) { }
};

class BackendFloatZero : public BackendLLVMSingle<llvm::ConstantFP>
{
public:
    BackendFloatZero() = default;
    BackendFloatZero(const FunctionWrap& wrap)
      : BackendLLVMSingle<llvm::ConstantFP>(wrap, [](llvm::ConstantFP& value) { return value.isZero(); }) { }
};

template<unsigned op>
class BackendOpcode : public BackendLLVMSingle<llvm::Instruction>
{
public:
    BackendOpcode() = default;
    BackendOpcode(const FunctionWrap& wrap)
      : BackendLLVMSingle<llvm::Instruction>(wrap, [](llvm::Instruction& inst) { return inst.getOpcode() == op; }) { }
};

template<bool(llvm::Type::*predicate)() const>
class BackendLLVMType: public BackendLLVMSingle<llvm::Value>
{
public:
    BackendLLVMType() = default;
    BackendLLVMType(const FunctionWrap& wrap)
      : BackendLLVMSingle<llvm::Value>(wrap, [](llvm::Value& value)
                                              { return (value.getType()->*predicate)(); }) { }
};

template<std::vector<std::vector<unsigned>> FunctionWrap::* forw_graph,
         std::vector<std::vector<unsigned>> FunctionWrap::* back_graph>
class BackendLLVMEdge : public BackendEdge
{
public:
    BackendLLVMEdge(const FunctionWrap& wrap)
      : BackendEdge(wrap.*forw_graph, wrap.*back_graph) { }
};

template<unsigned i>
class BackendLLVMOperand : public BackendEdge
{
public:
    BackendLLVMOperand(const FunctionWrap& wrap)
      : BackendEdge(wrap.odfg[i], wrap.rodfg[i]) { }
};

template<bool lt, bool eq, bool gt>
class BackendOrderWrap : public BackendOrdering<lt,eq,gt>
{
public:
    BackendOrderWrap(const FunctionWrap&)
      : BackendOrdering<lt,eq,gt>() { }
};

template<bool inverted, bool unstrict, unsigned origin_calc,
         std::vector<std::vector<unsigned>> FunctionWrap::* forw_graph>
class BackendLLVMDominate : public BackendDominate<inverted,unstrict>
{
public:
    BackendLLVMDominate(std::array<unsigned,3> sizes, const FunctionWrap& wrap)
      : BackendDominate<inverted,unstrict>({{(unsigned)std::get<0>(sizes) + (unsigned)get_origins(wrap).size(),
                                             (unsigned)std::get<1>(sizes), (unsigned)std::get<2>(sizes)}},
                                            wrap.*forw_graph),
        number_origins(get_origins(wrap).size())
    {
        std::vector<unsigned> origins = get_origins(wrap);

        for(unsigned i = 0; i < number_origins; i++)
        {
            BackendDominate<inverted,unstrict>::template begin<0>       (i);
            BackendDominate<inverted,unstrict>::template skip_invalid<0>(i, origins[i]);
            BackendDominate<inverted,unstrict>::template fixate<0>      (i, origins[i]);
        }
    }

    template<unsigned idx1> SkipResult skip_invalid(unsigned idx2, unsigned &c)
        { return BackendDominate<inverted,unstrict>::template skip_invalid<idx1>(idx2+(idx1?0:number_origins), c); }

    template<unsigned idx1> void begin(unsigned idx2)
        { BackendDominate<inverted,unstrict>::template begin<idx1>(idx2+(idx1?0:number_origins)); }

    template<unsigned idx1> void fixate(unsigned idx2, unsigned c)
        { BackendDominate<inverted,unstrict>::template fixate<idx1>(idx2+(idx1?0:number_origins), c); }

    template<unsigned idx1> void resume(unsigned idx2)
        { BackendDominate<inverted,unstrict>::template resume<idx1>(idx2+(idx1?0:number_origins)); }

private:
    static std::vector<unsigned> get_origins(const FunctionWrap& wrap)
    {
        bool data_origins    = origin_calc==0 || origin_calc==4;
        bool data_sinks      = origin_calc==1 || origin_calc==5;
        bool control_origins = origin_calc==2 || origin_calc==4;
        bool control_sinks   = origin_calc==3 || origin_calc==5;

        std::vector<unsigned> origins;

        for(unsigned i = 0; i < wrap.size(); i++)
        {
            if((data_sinks || data_origins) && llvm::isa<llvm::CallInst>(wrap[i]))
            {
                if(auto call_inst = llvm::dyn_cast<llvm::CallInst>(wrap[i]))
                {
                    if(call_inst->getCalledFunction() == nullptr)
                    {
                        //WOW, wait a second, this is overly optimistic!
                        continue;
                    }
                
                    auto attributes = call_inst->getCalledFunction()->getAttributes();

                    if(!(attributes.hasAttribute(llvm::AttributeList::FunctionIndex, llvm::Attribute::ReadOnly) ||
                         attributes.hasAttribute(llvm::AttributeList::FunctionIndex, llvm::Attribute::ReadNone)) ||
                       !attributes.hasAttribute(llvm::AttributeList::FunctionIndex, llvm::Attribute::NoUnwind))
                    {
                        origins.push_back(i);
                    }
                }
            }
            else if((data_origins    && llvm::isa<llvm::LoadInst>(wrap[i]))   ||
                    (data_origins    && llvm::isa<llvm::Argument>(wrap[i])) ||
                    (data_sinks      && llvm::isa<llvm::StoreInst>(wrap[i]))  ||
                    (data_sinks      && llvm::isa<llvm::ReturnInst>(wrap[i]))    ||
                    (control_origins && wrap.rcfg[i].empty() && !wrap.cfg[i].empty()) ||
                    (control_sinks   && wrap.cfg[i].empty()  && !wrap.rcfg[i].empty()))
            {
                origins.push_back(i);
            }
        }

        return origins;
    }

private:
    unsigned number_origins;
};

extern template class BackendConstantValue<UINT_MAX-1>;

extern template class BackendLLVMType<&llvm::Type::isIntegerTy>;
extern template class BackendLLVMType<&llvm::Type::isFloatTy>;
extern template class BackendLLVMType<&llvm::Type::isVectorTy>;
extern template class BackendLLVMType<&llvm::Type::isPointerTy>;

extern template class BackendOpcode<llvm::Instruction::PHI>;
extern template class BackendOpcode<llvm::Instruction::Store>;
extern template class BackendOpcode<llvm::Instruction::Load>;
extern template class BackendOpcode<llvm::Instruction::Ret>;
extern template class BackendOpcode<llvm::Instruction::Br>;
extern template class BackendOpcode<llvm::Instruction::Add>;
extern template class BackendOpcode<llvm::Instruction::Sub>;
extern template class BackendOpcode<llvm::Instruction::Mul>;
extern template class BackendOpcode<llvm::Instruction::FAdd>;
extern template class BackendOpcode<llvm::Instruction::FSub>;
extern template class BackendOpcode<llvm::Instruction::FMul>;
extern template class BackendOpcode<llvm::Instruction::FDiv>;
extern template class BackendOpcode<llvm::Instruction::Or>;
extern template class BackendOpcode<llvm::Instruction::And>;
extern template class BackendOpcode<llvm::Instruction::Shl>;
extern template class BackendOpcode<llvm::Instruction::Select>;
extern template class BackendOpcode<llvm::Instruction::SExt>;
extern template class BackendOpcode<llvm::Instruction::ZExt>;
extern template class BackendOpcode<llvm::Instruction::GetElementPtr>;
extern template class BackendOpcode<llvm::Instruction::ICmp>;
extern template class BackendOpcode<llvm::Instruction::Call>;
extern template class BackendOpcode<llvm::Instruction::ShuffleVector>;
extern template class BackendOpcode<llvm::Instruction::InsertElement>;

extern template class BackendOrderWrap<false,true,false>;
extern template class BackendOrderWrap<true,false,true>;
extern template class BackendOrderWrap<true,false,false>;

extern template class BackendLLVMEdge<&FunctionWrap::dfg, &FunctionWrap::rdfg>;
extern template class BackendLLVMEdge<&FunctionWrap::cfg, &FunctionWrap::rcfg>;
extern template class BackendLLVMEdge<&FunctionWrap::cdg, &FunctionWrap::rcdg>;
extern template class BackendLLVMEdge<&FunctionWrap::pdg, &FunctionWrap::rpdg>;

extern template class BackendLLVMOperand<0>;
extern template class BackendLLVMOperand<1>;
extern template class BackendLLVMOperand<2>;
extern template class BackendLLVMOperand<3>;

extern template class BackendLLVMDominate<false, true,  0, &FunctionWrap:: dfg>;
extern template class BackendLLVMDominate<false, true,  1, &FunctionWrap::rdfg>;
extern template class BackendLLVMDominate<false, false, 0, &FunctionWrap:: dfg>;
extern template class BackendLLVMDominate<false, false, 1, &FunctionWrap::rdfg>;

extern template class BackendLLVMDominate<false, true,  2, &FunctionWrap:: cfg>;
extern template class BackendLLVMDominate<false, true,  3, &FunctionWrap::rcfg>;
extern template class BackendLLVMDominate<false, false, 2, &FunctionWrap:: cfg>;
extern template class BackendLLVMDominate<false, false, 3, &FunctionWrap::rcfg>;

extern template class BackendLLVMDominate<false, true,  4, &FunctionWrap:: pdg>;
extern template class BackendLLVMDominate<false, true,  5, &FunctionWrap::rpdg>;
extern template class BackendLLVMDominate<false, false, 4, &FunctionWrap:: pdg>;
extern template class BackendLLVMDominate<false, false, 5, &FunctionWrap::rpdg>;

extern template class BackendLLVMDominate<true, true,  0, &FunctionWrap:: dfg>;
extern template class BackendLLVMDominate<true, true,  1, &FunctionWrap::rdfg>;
extern template class BackendLLVMDominate<true, false, 0, &FunctionWrap:: dfg>;
extern template class BackendLLVMDominate<true, false, 1, &FunctionWrap::rdfg>;

extern template class BackendLLVMDominate<true, true,  2, &FunctionWrap:: cfg>;
extern template class BackendLLVMDominate<true, true,  3, &FunctionWrap::rcfg>;
extern template class BackendLLVMDominate<true, false, 2, &FunctionWrap:: cfg>;
extern template class BackendLLVMDominate<true, false, 3, &FunctionWrap::rcfg>;

extern template class BackendLLVMDominate<true, true,  4, &FunctionWrap:: pdg>;
extern template class BackendLLVMDominate<true, true,  5, &FunctionWrap::rpdg>;
extern template class BackendLLVMDominate<true, false, 4, &FunctionWrap:: pdg>;
extern template class BackendLLVMDominate<true, false, 5, &FunctionWrap::rpdg>;

extern template class BackendLLVMDominate<false, false, UINT_MAX, &FunctionWrap::cfg>;
extern template class BackendLLVMDominate<false, false, UINT_MAX, &FunctionWrap::dfg>;
extern template class BackendLLVMDominate<false, false, UINT_MAX, &FunctionWrap::pdg>;

using BackendUnused = BackendConstantValue<UINT_MAX-1>;

using BackendIntegerType = BackendLLVMType<&llvm::Type::isIntegerTy>;
using BackendFloatType   = BackendLLVMType<&llvm::Type::isFloatTy>;
using BackendVectorType  = BackendLLVMType<&llvm::Type::isVectorTy>;
using BackendPointerType = BackendLLVMType<&llvm::Type::isPointerTy>;

using BackendPHIInst           = BackendOpcode<llvm::Instruction::PHI>;
using BackendStoreInst         = BackendOpcode<llvm::Instruction::Store>;
using BackendLoadInst          = BackendOpcode<llvm::Instruction::Load>;
using BackendReturnInst        = BackendOpcode<llvm::Instruction::Ret>;
using BackendBranchInst        = BackendOpcode<llvm::Instruction::Br>;
using BackendAddInst           = BackendOpcode<llvm::Instruction::Add>;
using BackendSubInst           = BackendOpcode<llvm::Instruction::Sub>;
using BackendMulInst           = BackendOpcode<llvm::Instruction::Mul>;
using BackendFAddInst          = BackendOpcode<llvm::Instruction::FAdd>;
using BackendFSubInst          = BackendOpcode<llvm::Instruction::FSub>;
using BackendFMulInst          = BackendOpcode<llvm::Instruction::FMul>;
using BackendFDivInst          = BackendOpcode<llvm::Instruction::FDiv>;
using BackendBitOrInst         = BackendOpcode<llvm::Instruction::Or>;
using BackendBitAndInst        = BackendOpcode<llvm::Instruction::And>;
using BackendLShiftInst        = BackendOpcode<llvm::Instruction::Shl>;
using BackendSelectInst        = BackendOpcode<llvm::Instruction::Select>;
using BackendSExtInst          = BackendOpcode<llvm::Instruction::SExt>;
using BackendZExtInst          = BackendOpcode<llvm::Instruction::ZExt>;
using BackendGEPInst           = BackendOpcode<llvm::Instruction::GetElementPtr>;
using BackendICmpInst          = BackendOpcode<llvm::Instruction::ICmp>;
using BackendCallInst          = BackendOpcode<llvm::Instruction::Call>;
using BackendShufflevectorInst = BackendOpcode<llvm::Instruction::ShuffleVector>;
using BackendInsertelementInst = BackendOpcode<llvm::Instruction::InsertElement>;

using BackendSame     = BackendOrderWrap<false,true,false>;
using BackendDistinct = BackendOrderWrap<true,false,true>;
using BackendOrder    = BackendOrderWrap<true,false,false>;

using BackendDFGEdge  = BackendLLVMEdge<&FunctionWrap::dfg, &FunctionWrap::rdfg>;
using BackendCFGEdge  = BackendLLVMEdge<&FunctionWrap::cfg, &FunctionWrap::rcfg>;
using BackendCDGEdge  = BackendLLVMEdge<&FunctionWrap::cdg, &FunctionWrap::rcdg>;
using BackendPDGEdge  = BackendLLVMEdge<&FunctionWrap::pdg, &FunctionWrap::rpdg>;

using BackendFirstOperand  = BackendLLVMOperand<0>;
using BackendSecondOperand = BackendLLVMOperand<1>;
using BackendThirdOperand  = BackendLLVMOperand<2>;
using BackendFourthOperand = BackendLLVMOperand<3>;

using BackendDFGDominate       = BackendLLVMDominate<false, true,  0, &FunctionWrap:: dfg>;
using BackendDFGPostdom        = BackendLLVMDominate<false, true,  1, &FunctionWrap::rdfg>;
using BackendDFGDominateStrict = BackendLLVMDominate<false, false, 0, &FunctionWrap:: dfg>;
using BackendDFGPostdomStrict  = BackendLLVMDominate<false, false, 1, &FunctionWrap::rdfg>;

using BackendCFGDominate       = BackendLLVMDominate<false, true,  2, &FunctionWrap:: cfg>;
using BackendCFGPostdom        = BackendLLVMDominate<false, true,  3, &FunctionWrap::rcfg>;
using BackendCFGDominateStrict = BackendLLVMDominate<false, false, 2, &FunctionWrap:: cfg>;
using BackendCFGPostdomStrict  = BackendLLVMDominate<false, false, 3, &FunctionWrap::rcfg>;

using BackendPDGDominate       = BackendLLVMDominate<false, true,  4, &FunctionWrap:: pdg>;
using BackendPDGPostdom        = BackendLLVMDominate<false, true,  5, &FunctionWrap::rpdg>;
using BackendPDGDominateStrict = BackendLLVMDominate<false, false, 4, &FunctionWrap:: pdg>;
using BackendPDGPostdomStrict  = BackendLLVMDominate<false, false, 5, &FunctionWrap::rpdg>;

using BackendDFGNotDominate       = BackendLLVMDominate<true, true,  0, &FunctionWrap:: dfg>;
using BackendDFGNotPostdom        = BackendLLVMDominate<true, true,  1, &FunctionWrap::rdfg>;
using BackendDFGNotDominateStrict = BackendLLVMDominate<true, false, 0, &FunctionWrap:: dfg>;
using BackendDFGNotPostdomStrict  = BackendLLVMDominate<true, false, 1, &FunctionWrap::rdfg>;

using BackendCFGNotDominate       = BackendLLVMDominate<true, true,  2, &FunctionWrap:: cfg>;
using BackendCFGNotPostdom        = BackendLLVMDominate<true, true,  3, &FunctionWrap::rcfg>;
using BackendCFGNotDominateStrict = BackendLLVMDominate<true, false, 2, &FunctionWrap:: cfg>;
using BackendCFGNotPostdomStrict  = BackendLLVMDominate<true, false, 3, &FunctionWrap::rcfg>;

using BackendPDGNotDominate       = BackendLLVMDominate<true, true,  4, &FunctionWrap:: pdg>;
using BackendPDGNotPostdom        = BackendLLVMDominate<true, true,  5, &FunctionWrap::rpdg>;
using BackendPDGNotDominateStrict = BackendLLVMDominate<true, false, 4, &FunctionWrap:: pdg>;
using BackendPDGNotPostdomStrict  = BackendLLVMDominate<true, false, 5, &FunctionWrap::rpdg>;

using BackendCFGBlocked = BackendLLVMDominate<false, false, UINT_MAX, &FunctionWrap::cfg>;
using BackendDFGBlocked = BackendLLVMDominate<false, false, UINT_MAX, &FunctionWrap::dfg>;
using BackendPDGBlocked = BackendLLVMDominate<false, false, UINT_MAX, &FunctionWrap::pdg>;

#endif
