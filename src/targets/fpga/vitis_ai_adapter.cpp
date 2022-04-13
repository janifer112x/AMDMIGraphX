#include "migraphx/fpga/vitis_ai_adapter.hpp"

#include "migraphx/module.hpp"
#include <migraphx/register_op.hpp>
#include "migraphx/stringutils.hpp"

namespace migraphx {
inline namespace MIGRAPHX_INLINE_NS {

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

    // argument compute(const shape& output_shape, std::vector<argument> args) const
    // {
    //     return ::vitis_ai::execute(xmodel_, output_shape, args);
    // }
};
MIGRAPHX_REGISTER_OP(fpga_placeholder_op)
} // namespace fpga
} // namespace MIGRAPHX_INLINE_NS
} // namespace migraphx

namespace vitis_ai {

migraphx::shape XModel::get_shape() const { return shape_; };

void XModel::set_shape(migraphx::shape shape) { shape_ = shape; }

bool is_fpga_instr(migraphx::instruction_ref it)
{
    return (!it->inputs().empty()) && (!migraphx::starts_with(it->name(), "@"));
}

XModel create_xmodel(migraphx::module_ref mod)
{
    std::cout << "Calling an external function: create_xmodel!\n";
    XModel xmodel;
    xmodel.set_shape(std::prev(mod->end())->get_shape());
    return xmodel;
}

migraphx::argument
execute(XModel xmodel, const migraphx::shape& output_shape, std::vector<migraphx::argument>& args)
{
    (void)xmodel;

    std::cout << "Calling an external function: execute!\n";

    std::cout << "Output Shape: " << output_shape << std::endl;
    std::cout << "Args: " << args.size() << std::endl;
    for(const auto& arg : args)
    {
        std::cout << "  " << arg.get_shape() << std::endl;
    }
    std::cout << std::endl;

    migraphx::argument result{output_shape};

    return result;
}

} // namespace vitis_ai
