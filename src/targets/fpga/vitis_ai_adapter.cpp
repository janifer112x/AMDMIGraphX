#include "migraphx/fpga/vitis_ai_adapter.hpp"

#include "migraphx/module.hpp"

#include "migraphx/stringutils.hpp"
namespace vitis_ai {

migraphx::shape XModel::get_shape() const { return shape_; };

void XModel::set_shape(migraphx::shape shape) { shape_ = shape; }

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
