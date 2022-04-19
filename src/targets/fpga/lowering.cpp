
#include <migraphx/fpga/lowering.hpp>
#include <migraphx/instruction.hpp>
#include <migraphx/iterator_for.hpp>
#include <migraphx/register_op.hpp>
#include <migraphx/stringutils.hpp>
#include <iostream>

#include "migraphx/fpga/vitis_ai_adapter.hpp"

namespace migraphx {
inline namespace MIGRAPHX_INLINE_NS {

namespace fpga {

struct fpga_vitis_op
{
    fpga_vitis_op() = default;
    explicit fpga_vitis_op(vitis_ai::XModel xmodel) : xmodel_(xmodel){};

    vitis_ai::XModel xmodel_;
    int dummy = 0;

    template <class Self, class F>
    static auto reflect(Self& self, F f)
    {
        // return pack(f(self.xmodel_, "xmodel"));
        return pack(f(self.dummy, "dummy"));
    }

    std::string name() const { return "fpga::vitis_ai"; }

    shape compute_shape(std::vector<shape> inputs) const
    {
        (void)inputs;
        return xmodel_.get_shape();
    }

    argument compute(context& ctx, const shape& output_shape, std::vector<argument> args) const
    {
        std::cout << "The context is " << ctx.foo << std::endl;
        return ::vitis_ai::execute(xmodel_, output_shape, args);
    }
};
MIGRAPHX_REGISTER_OP(fpga_vitis_op)

void lowering::apply(module& m) const
{
    auto* mod = &m;

    // test modifying the context from a pass
    ctx->foo = 2;

    for(auto it : iterator_for(*mod))
    {
        if(it->name() == "fpga::vitis_placeholder")
        {
            assert(it->module_inputs().size() == 1);
            auto xmodel = ::vitis_ai::create_xmodel(it->module_inputs()[0]);
            mod->replace_instruction(it, fpga_vitis_op{xmodel}, it->inputs());
        }
    }
}

} // namespace fpga
} // namespace MIGRAPHX_INLINE_NS
} // namespace migraphx
