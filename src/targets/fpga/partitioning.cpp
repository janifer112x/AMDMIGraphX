
#include <migraphx/fpga/partitioning.hpp>

#include <migraphx/instruction.hpp>
#include "migraphx/iterator.hpp"
#include <migraphx/iterator_for.hpp>
#include "migraphx/make_op.hpp"
#include "migraphx/ranges.hpp"
#include <migraphx/register_op.hpp>
#include <migraphx/stringutils.hpp>
#include <migraphx/pass_manager.hpp>

namespace migraphx {
inline namespace MIGRAPHX_INLINE_NS {

// this is taken from the insert-instruction branch. In time, this will be
// merged into the migraphx API
template <class Range>
static std::vector<instruction_ref>
insert_generic_instructions(module& m,
                            instruction_ref ins,
                            Range&& instructions,
                            std::unordered_map<instruction_ref, instruction_ref> map_ins)
{
    assert(m.has_instruction(ins) or is_end(ins, m.end()));
    std::vector<instruction_ref> mod_outputs;
    instruction_ref last;
    for(instruction_ref sins : instructions)
    {
        last = sins;
        if(contains(map_ins, sins))
            continue;
        instruction_ref copy_ins;
        if(sins->name() == "@literal")
        {
            auto l   = sins->get_literal();
            copy_ins = m.add_literal(l);
        }
        else if(sins->name() == "@param")
        {
            auto&& name = any_cast<builtin::param>(sins->get_operator()).parameter;
            auto s      = sins->get_shape();
            copy_ins    = m.add_parameter(name, s);
        }
        else if(sins->name() == "@outline")
        {
            auto s   = sins->get_shape();
            copy_ins = m.add_outline(s);
        }
        else
        {
            auto mod_args = sins->module_inputs();
            auto inputs   = sins->inputs();
            std::vector<instruction_ref> copy_inputs(inputs.size());
            std::transform(inputs.begin(), inputs.end(), copy_inputs.begin(), [&](auto i) {
                return contains(map_ins, i) ? map_ins[i] : i;
            });

            if(sins->name() == "@return")
            {
                mod_outputs = copy_inputs;
                break;
            }

            copy_ins = m.insert_instruction(ins, sins->get_operator(), copy_inputs, mod_args);
        }
        map_ins[sins] = copy_ins;
    }
    if(mod_outputs.empty() and instructions.begin() != instructions.end())
        mod_outputs = {map_ins.at(last)};
    return mod_outputs;
}

namespace fpga {

struct fpga_placeholder_op
{
    fpga_placeholder_op() = default;

    int dummy = 0;

    template <class Self, class F>
    static auto reflect(Self& self, F f)
    {
        return pack(f(self.dummy, "dummy"));
    }

    std::string name() const { return "fpga::vitis_placeholder"; }

    shape compute_shape(const std::vector<shape>& inputs, std::vector<module_ref> mods) const
    {
        (void)inputs;
        if(mods.size() != 1)
        {
            MIGRAPHX_THROW("should have one submodule.");
        }
        module_ref sm = mods.front();
        if(sm->get_output_shapes().size() != 1)
            MIGRAPHX_THROW("Only one return");
        return sm->get_output_shapes().front();
    }
};
MIGRAPHX_REGISTER_OP(fpga_placeholder_op)

bool is_fpga_instr(migraphx::instruction_ref it)
{
    return (!it->inputs().empty()) || (migraphx::starts_with(it->name(), "@"));
}

void partitioning::apply(module_pass_manager& mpm) const
{
    auto& mod = mpm.get_module();
    auto* pm  = mpm.create_module(mod.name() + ":fpga");
    pm->set_bypass();

    migraphx::instruction_ref first = mod.end();
    migraphx::instruction_ref last;
    std::vector<migraphx::instruction_ref> literal_inputs;
    for(auto it : iterator_for(mod))
    {
        // assuming we want all the params/literals as inputs to the FPGA submodule
        if(migraphx::starts_with(it->name(), "@param") ||
           migraphx::starts_with(it->name(), "@literal"))
        {
            literal_inputs.push_back(it);
        }
        if(is_fpga_instr(it))
        {
            if(first == mod.end())
            {
                first = it;
            }
            last = it;
        }
    }

    // assuming all FPGA instructions are in one contiguous range
    auto r = migraphx::range(first, last);
    migraphx::insert_generic_instructions(*pm, pm->end(), migraphx::iterator_for(r), {});

    migraphx::instruction_ref placeholder_ins;
    for(auto it : iterator_for(mod))
    {
        if(migraphx::starts_with(it->name(), "@return"))
        {
            placeholder_ins = mod.insert_instruction(
                it, migraphx::make_op("fpga::vitis_placeholder"), literal_inputs, {pm});
            break;
        }
    }

    mod.replace_return({placeholder_ins});
}

} // namespace fpga
} // namespace MIGRAPHX_INLINE_NS
} // namespace migraphx
