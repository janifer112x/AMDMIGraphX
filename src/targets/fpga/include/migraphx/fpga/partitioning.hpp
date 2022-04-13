#ifndef MIGRAPHX_GUARD_RTGLIB_FPGA_PARTITIONING_HPP
#define MIGRAPHX_GUARD_RTGLIB_FPGA_PARTITIONING_HPP

#include <migraphx/program.hpp>
#include <migraphx/config.hpp>

namespace migraphx {
inline namespace MIGRAPHX_INLINE_NS {
namespace fpga {

struct partitioning
{
    std::string name() const { return "fpga::partitioning"; }
    void apply(module_pass_manager& mpm) const;
};

} // namespace fpga
} // namespace MIGRAPHX_INLINE_NS
} // namespace migraphx

#endif
