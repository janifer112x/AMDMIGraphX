#pragma once

#include <string>

#include <migraphx/instruction.hpp>
#include <migraphx/pass_manager.hpp>

namespace vitis_ai {

class XModel {
 public:
  migraphx::shape get_shape() const;
  void set_shape(migraphx::shape);

 private:
  migraphx::shape shape_;
};

XModel create_xmodel(migraphx::module_ref mod);

migraphx::argument execute(XModel xmodel, const migraphx::shape& output_shape, std::vector<migraphx::argument>& args);



} // namespace vitis_ai
