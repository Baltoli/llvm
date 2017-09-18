#include "llvm/Constraints/BackendClasses.hpp"
#include <memory>
#include <vector>
#include <limits>

BackendOr::BackendOr(std::array<unsigned,1>, std::vector<std::vector<std::unique_ptr<SolverAtom>>> c)
          : constraints(std::move(c)), disabled_since(constraints.empty()?0:constraints.front().size(), 0) { }

SkipResult BackendOr::skip_invalid(unsigned idx2, SolverAtom::Value& c) const
{
    SkipResult result = SkipResult::FAIL;
    SolverAtom::Value min = std::numeric_limits<SolverAtom::Value>::max();

    for(unsigned i = 0; i < constraints[idx2].size(); i++)
    {
        unsigned temp = c;
        SkipResult local_result = (disabled_since[i] > 0) ? SkipResult::FAIL : constraints[idx2][i]->skip_invalid(temp);

        if(local_result == SkipResult::PASS)
        {
            result = SkipResult::PASS;
            break;
        }
        else if(local_result == SkipResult::CHANGE || local_result == SkipResult::CHANGEPASS)
        {
            result = SkipResult::CHANGE;

            if(temp < min)
            {
                min = temp;
            }
        }
    }

    if(result == SkipResult::CHANGE)
    {
        c = min;
    }

    return result;
}

void BackendOr::begin(unsigned idx2)
{
    for(unsigned i = 0; i < constraints[idx2].size(); i++)
    {
        if(disabled_since[i] == 0)
        {
            constraints[idx2][i]->begin();
        }
    }
}

void BackendOr::fixate(unsigned idx2, SolverAtom::Value c)
{
    for(unsigned i = 0; i < constraints[idx2].size(); i++)
    {
        if(disabled_since[i] > 0) disabled_since[i] ++;
        else
        {
            SolverAtom::Value temp = c;

            if(constraints[idx2][i]->skip_invalid(temp) != SkipResult::PASS)
            {
                disabled_since[i] = 1;
            }
            else
            {
                constraints[idx2][i]->fixate(c);
            }
        }
    }
}

void BackendOr::resume(unsigned idx2)
{
    for(unsigned i = 0; i < constraints[idx2].size(); i++)
    {
        if(disabled_since[i] > 0)
        {
            disabled_since[i]--;
        }
        else
        {
            constraints[idx2][i]->resume();
        }
    }
}
