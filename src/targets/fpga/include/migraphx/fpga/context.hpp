#ifndef MIGRAPHX_GUARD_RTGLIB_CONTEXT_HPP
#define MIGRAPHX_GUARD_RTGLIB_CONTEXT_HPP

#include <migraphx/config.hpp>

namespace migraphx {
inline namespace MIGRAPHX_INLINE_NS {
namespace fpga {

struct context
{
    void finish() const {}
};

} // namespace fpga
} // namespace MIGRAPHX_INLINE_NS
} // namespace migraphx

#endif
