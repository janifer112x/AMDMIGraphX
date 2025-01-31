#ifndef MIGRAPHX_GUARD_KERNELS_REDUCE_HPP
#define MIGRAPHX_GUARD_KERNELS_REDUCE_HPP

#include <migraphx/kernels/dpp.hpp>
#include <migraphx/kernels/index.hpp>
#include <migraphx/kernels/tensor_view.hpp>
#include <migraphx/kernels/ops.hpp>

namespace migraphx {

#if MIGRAPHX_HAS_DPP

template <class T, class Op>
__device__ void dpp_reduce(T& in, Op op)
{
    T out{};
    out = dpp_mov<dpp_row_shr(1)>(in);
    in  = op(in, out);
    out = dpp_mov<dpp_row_shr(2)>(in);
    in  = op(in, out);
    out = dpp_mov<dpp_row_shr(4), 0xf, 0xe>(in);
    in  = op(in, out);
    out = dpp_mov<dpp_row_shr(8), 0xf, 0xc>(in);
    in  = op(in, out);
#if __AMDGCN_WAVEFRONT_SIZE == 64
    out = dpp_mov<dpp_row_bcast(15), 0xa>(in);
    in  = op(in, out);
    out = dpp_mov<dpp_row_bcast(31), 0xc>(in);
    in  = op(in, out);
#endif
}
#if defined(MIGRAPHX_USE_CLANG_TIDY) || defined(CPPCHECK)
// NOLINTNEXTLINE
#define MIGRAPHX_DPP_REDUCE_ASM(x, ins) x = 1
#elif __AMDGCN_WAVEFRONT_SIZE == 64
#define MIGRAPHX_DPP_REDUCE_ASM(x, ins)                                       \
    __asm__ volatile("s_nop 4\n" #ins " %0 %0 %0 row_shr:1\n"                 \
                     "s_nop 1\n" #ins " %0 %0 %0 row_shr:2\n"                 \
                     "s_nop 1\n" #ins " %0 %0 %0 row_shr:4 bank_mask:0xe\n"   \
                     "s_nop 1\n" #ins " %0 %0 %0 row_shr:8 bank_mask:0xc\n"   \
                     "s_nop 1\n" #ins " %0 %0 %0 row_bcast:15 row_mask:0xa\n" \
                     "s_nop 1\n" #ins " %0 %0 %0 row_bcast:31 row_mask:0xc\n" \
                     "s_nop 1\n"                                              \
                     : "=v"(x)                                                \
                     : "0"(x))
#else
#define MIGRAPHX_DPP_REDUCE_ASM(x, ins)                                     \
    __asm__ volatile("s_nop 4\n" #ins " %0 %0 %0 row_shr:1\n"               \
                     "s_nop 1\n" #ins " %0 %0 %0 row_shr:2\n"               \
                     "s_nop 1\n" #ins " %0 %0 %0 row_shr:4 bank_mask:0xe\n" \
                     "s_nop 1\n" #ins " %0 %0 %0 row_shr:8 bank_mask:0xc\n" \
                     "s_nop 1\n"                                            \
                     "s_nop 1\n"                                            \
                     : "=v"(x)                                              \
                     : "0"(x))
#endif

// NOLINTNEXTLINE
#define MIGRAPHX_DPP_REDUCE(op, prefix)                                                            \
    __device__ inline void dpp_reduce(double& x, op) { MIGRAPHX_DPP_REDUCE_ASM(x, prefix##_f64); } \
    __device__ inline void dpp_reduce(float& x, op) { MIGRAPHX_DPP_REDUCE_ASM(x, prefix##_f32); }  \
    __device__ inline void dpp_reduce(half& x, op) { MIGRAPHX_DPP_REDUCE_ASM(x, prefix##_f16); }   \
    __device__ inline void dpp_reduce(int32_t& x, op)                                              \
    {                                                                                              \
        MIGRAPHX_DPP_REDUCE_ASM(x, prefix##_u32);                                                  \
    }                                                                                              \
    __device__ inline void dpp_reduce(uint32_t& x, op) { MIGRAPHX_DPP_REDUCE_ASM(x, prefix##_u32); }

MIGRAPHX_DPP_REDUCE(op::sum, v_add)
MIGRAPHX_DPP_REDUCE(op::max, v_max)
MIGRAPHX_DPP_REDUCE(op::min, v_min)
MIGRAPHX_DPP_REDUCE(op::product, v_mul)

template <class Op, class T, class F>
__device__ auto block_reduce(index idx, Op op, T init, index_int n, F f)
{
#if __AMDGCN_WAVEFRONT_SIZE == 32
    constexpr index_int lanes_per_thread = 16;
#else
    constexpr index_int lanes_per_thread = 64;
#endif
    using type = decltype(f(0));
    __shared__ type buffer[idx.nlocal() / lanes_per_thread];
    type x = init;
    idx.local_stride(n, [&](auto i) { x = op(x, f(i)); });
    dpp_reduce(x, op);

    const auto ldsidx = idx.local / lanes_per_thread;
    if((idx.local % lanes_per_thread) == lanes_per_thread - 1)
    {
        buffer[ldsidx] = x;
    }
    __syncthreads();

    type y = init;
    for(index_int i = 0; i < idx.nlocal() / lanes_per_thread; i++)
    {
        y = op(y, buffer[i]);
    }
    return y;
}
#else
template <class Op, class T, class F>
__device__ auto block_reduce(index idx, Op op, T init, index_int n, F f)
{

    using type = decltype(f(0));
    __shared__ type buffer[idx.nlocal()];
    type x = init;
    idx.local_stride(n, [&](auto i) { x = op(x, f(i)); });
    buffer[idx.local] = x;
    __syncthreads();

    for(index_int s = 1; s < idx.nlocal(); s *= 2)
    {
        const index_int index = 2 * s * idx.local;
        if(index + s < idx.nlocal())
        {
            buffer[index] = op(buffer[index], buffer[index + s]);
        }
        __syncthreads();
    }
    return buffer[0];
}
#endif

template <class Input, class T, class Output>
constexpr auto reduce_slice(Input input, T i, Output)
{
    constexpr auto lens = transform(get_shape_c<Input>{}.lens,
                                    get_shape_c<Output>{}.lens,
                                    [](index_int x, index_int y) -> index_int {
                                        if(x == y)
                                            return 1;
                                        return x;
                                    });
    ;
    constexpr auto s = make_shape(lens, get_shape_c<Input>{}.strides);
    return make_tensor_view(&input[i], s);
}

template <class Op, class T, class Input, class Output, class ReadInput, class WriteOuput>
__device__ void
simple_reduce(Op op, T init, Input input, Output output, ReadInput read, WriteOuput write)
{
    auto idx                 = make_index();
    constexpr auto nelements = get_shape_c<Output>{}.elements();
    constexpr auto relements = get_shape_c<Input>{}.elements() / get_shape_c<Output>{}.elements();
    idx.global_stride(nelements * idx.nlocal(), [&](auto i) {
        const auto out_idx = output.get_shape().multi(i / idx.nlocal());
        auto rs            = reduce_slice(input, out_idx, output);
        MIGRAPHX_ASSERT(relements == rs.get_shape().elements());
        auto r = block_reduce(idx, op, init, relements, [&](auto j) { return read(rs[j]); });
        if(idx.local == 0)
            output[out_idx] = write(r);
    });
}

} // namespace migraphx
#endif // MIGRAPHX_GUARD_KERNELS_REDUCE_HPP
