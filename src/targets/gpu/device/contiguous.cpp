
#include <migraph/gpu/device/contiguous.hpp>
#include <migraph/gpu/device/launch.hpp>

namespace migraph {
namespace gpu {
namespace device {

template <class F>
void visit_tensor_size(std::size_t n, F f)
{
    switch(n)
    {
    case 1:
    {
        f(std::integral_constant<std::size_t, 1>{});
        break;
    }
    case 2:
    {
        f(std::integral_constant<std::size_t, 2>{});
        break;
    }
    case 3:
    {
        f(std::integral_constant<std::size_t, 3>{});
        break;
    }
    case 4:
    {
        f(std::integral_constant<std::size_t, 4>{});
        break;
    }
    case 5:
    {
        f(std::integral_constant<std::size_t, 5>{});
        break;
    }
    default: throw std::runtime_error("Unknown tensor size");
    }
}

template <size_t NDim>
struct hip_index
{
    size_t d[NDim];
    __device__ __host__ size_t& operator[](size_t i) { return d[i]; }
    __device__ __host__ size_t operator[](size_t i) const { return d[i]; }
};

template <size_t NDim>
struct hip_tensor_descriptor
{
    __device__ __host__ hip_tensor_descriptor() = default;
    template <typename T, typename V>
    __device__ __host__ hip_tensor_descriptor(const T& lens_ext, const V& strides_ext)
    {
        for(size_t i = 0; i < NDim; i++)
            lens[i] = lens_ext[i];
        for(size_t i = 0; i < NDim; i++)
            strides[i] = strides_ext[i];
    }
    __device__ __host__ hip_index<NDim> multi(size_t idx) const
    {
        hip_index<NDim> result{};
        size_t tidx = idx;
        for(size_t is = 0; is < NDim; is++)
        {
            result[is] = tidx / strides[is];
            tidx       = tidx % strides[is];
        }
        return result;
    }
    __device__ __host__ size_t linear(hip_index<NDim> s) const
    {
        size_t idx = 0;
        for(size_t i = 0; i < NDim; i++)
            idx += s[i] * strides[i];
        return idx;
    }
    size_t lens[NDim]    = {};
    size_t strides[NDim] = {};
};

void contiguous(shape output_shape, argument arg, argument result)
{
    visit_all(result, arg)([&](auto output, auto input) {
        visit_tensor_size(output_shape.lens().size(), [&](auto ndim) {
            const auto& s = arg.get_shape();
            hip_tensor_descriptor<ndim> a_desc(s.lens(), s.strides());
            hip_tensor_descriptor<ndim> at_desc(output_shape.lens(), output_shape.strides());
            auto* a             = input.data();
            auto* at            = output.data();
            gs_launch(s.elements())([=](auto i) {
                size_t lidx = a_desc.linear(at_desc.multi(i));
                at[i]       = a[lidx];
            });
        });
    });
}
} // namespace device
} // namespace gpu
} // namespace migraph
