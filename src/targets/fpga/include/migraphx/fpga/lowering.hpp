#ifndef MIGRAPHX_GUARD_RTGLIB_FPGA_LOWERING_HPP
#define MIGRAPHX_GUARD_RTGLIB_FPGA_LOWERING_HPP

#include <migraphx/program.hpp>
#include <migraphx/config.hpp>
#include <migraphx/fpga/context.hpp>

namespace migraphx {
inline namespace MIGRAPHX_INLINE_NS {
namespace fpga {

struct lowering
{
    context* ctx = nullptr;
    std::string name() const { return "fpga::lowering"; }
    void apply(module& m) const;
};

} // namespace fpga
} // namespace MIGRAPHX_INLINE_NS
} // namespace migraphx

#endif
