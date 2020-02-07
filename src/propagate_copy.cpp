#include <migraphx/propagate_copy.hpp>
#include <migraphx/program.hpp>
#include <migraphx/instruction.hpp>
#include <migraphx/iterator_for.hpp>
#include <migraphx/op/identity.hpp>
#include <unordered_set>

namespace migraphx {
inline namespace MIGRAPHX_INLINE_NS {

bool single_use(instruction_ref alias, instruction_ref ins)
{
    if(ins->outputs().size() > 1)
        return false;
    if(alias == ins)
        return true;
    for(auto input : ins->inputs())
    {
        auto i = instruction::get_output_alias(input);
        if(i != alias)
            continue;
        if(not single_use(i, input))
            return false;
    }
    return true;
}

void propagate_copy::apply(program& p) const
{
    for(auto ins : iterator_for(p))
    {
        if(ins->name() != copy)
            continue;
        auto input = ins->inputs().front();
        if(ins->get_shape() != input->get_shape())
            continue;
        auto i = instruction::get_output_alias(input);
        if(i->name()[0] == '@')
            continue;
        if(not single_use(i, input))
            continue;
        p.replace_instruction(ins, op::identity{}, i, input);
    }
}

} // namespace MIGRAPHX_INLINE_NS
} // namespace migraphx
