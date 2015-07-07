// Copyright: 2014, 2015, Ableton AG, Berlin. All rights reserved.

#pragma once

#include <atria/xform/config.hpp>
#include <atria/xform/functional.hpp>
#include <atria/xform/state_traits.hpp>

#include <atria/estd/type_traits.hpp>
#include <atria/estd/utility.hpp>

#include <algorithm>
#include <numeric>
#include <tuple>

namespace atria {
namespace xform {

namespace detail {

template <typename ReducingFnT,
          typename StateT,
          typename InputRangeT>
auto reduce_nested_accumulate(ReducingFnT&& step, StateT&& state, InputRangeT&& range)
  -> estd::decay_t<StateT>
{
  return std::accumulate(
    std::begin(range),
    std::end(range),
    std::forward<StateT>(state),
    std::forward<ReducingFnT>(step));
}

template <typename ReducingFnT,
          typename StateT,
          typename InputIterT>
auto reduce_nested_tail_recursive_impl(ReducingFnT&& step,
                                       StateT&& state,
                                       InputIterT&& first,
                                       InputIterT&& last)
  -> estd::decay_t<decltype(step(state, *first))>
{
  using result_t = estd::decay_t<decltype(step(state, *first))>;

  if (state_is_reduced(state) || first == last) {
    return result_t { std::forward<StateT>(state) };
  }
  auto next_state = step(std::forward<StateT>(state), *first);
  return reduce_nested_tail_recursive_impl(
    std::forward<ReducingFnT>(step),
    std::move(next_state),
    std::forward<InputIterT>(++first),
    std::forward<InputIterT>(last));
}

template <typename ReducingFnT,
          typename StateT,
          typename InputRangeT>
auto reduce_nested_tail_recursive(ReducingFnT&& step,
                                  StateT&& initial,
                                  InputRangeT&& range)
  -> ABL_DECLTYPE_RETURN(
    reduce_nested_tail_recursive_impl(
      std::forward<ReducingFnT>(step),
      std::forward<StateT>(initial),
      std::begin(range),
      std::end(range)))

template <typename ReducingFnT,
          typename StateT,
          typename InputRangeT>
auto reduce_nested_non_variadic(ReducingFnT&& step,
                                StateT&& initial,
                                InputRangeT&& range)
  -> estd::decay_t<decltype(step(initial, *std::begin(range)))>
{
  using result_t = estd::decay_t<decltype(step(initial, *std::begin(range)))>;

  auto first = std::begin(range);
  auto last  = std::end(range);
  if (first != last) {
    auto state = step(std::forward<StateT>(initial), *first);
    // This may be expressed more brief with a:
    //    while(++first != last)
    // but the for loop seems to make compilers generate better code.
    for (++first; !state_is_reduced(state) && first != last; ++first) {
      // `x = std::move(x)` is undefined behaviour, hence the two
      // steps approach to protect for when `step` just forwards
      // the state back.
      auto new_state = step(std::move(state), *first);
      state = std::move(new_state);
    }
    return state;
  }

  return result_t { std::forward<StateT>(initial) };
}

template <typename ReducingFnT,
          typename StateT,
          std::size_t ...Indices,
          typename ...InputRangeTs>
auto reduce_nested_variadic_impl(ReducingFnT&& step,
                                 StateT&& initial,
                                 estd::index_sequence<Indices...>,
                                 InputRangeTs&& ...ranges)
  -> estd::decay_t<decltype(step(initial, *std::begin(ranges)...))>
{
  using result_t = estd::decay_t<decltype(
    step(initial, *std::begin(ranges)...))>;

  auto firsts = std::make_tuple(std::begin(ranges)...);
  auto lasts  = std::make_tuple(std::end(ranges)...);
  if (std::min({std::get<Indices>(firsts) !=
                std::get<Indices>(lasts)...}))
  {
    auto state = step(std::forward<StateT>(initial),
                         *std::get<Indices>(firsts)...);
    while (!state_is_reduced(state)
           && std::min({ ++std::get<Indices>(firsts) !=
                         std::get<Indices>(lasts)... })) {
      state = step(std::move(state), *std::get<Indices>(firsts)...);
    }
    return state;
  }

  return result_t { initial };
}

template <typename ReducingFnT,
          typename StateT,
          typename ...InputRangeTs>
auto reduce_nested_variadic(ReducingFnT&& step, StateT&& state, InputRangeTs&& ...ranges)
  -> ABL_DECLTYPE_RETURN(
    reduce_nested_variadic_impl(
      std::forward<ReducingFnT>(step),
      std::forward<StateT>(state),
      estd::make_index_sequence<sizeof...(InputRangeTs)> {},
      std::forward<InputRangeTs>(ranges)...))

} // namespace detail


#if ABL_REDUCE_TAIL_RECURSIVE
#  define ABL_REDUCE_NESTED_NON_VARIADIC_IMPL \
  ::atria::xform::detail::reduce_nested_tail_recursive
#elif ABL_REDUCE_WITH_ACCUMULATE
#  define ABL_REDUCE_NESTED_NON_VARIADIC_IMPL \
  ::atria::xform::detail::reduce_nested_accumulate
#elif ABL_REDUCE_ALWAYS_VARIADIC
#  define ABL_REDUCE_NESTED_NON_VARIADIC_IMPL \
  ::atria::xform::detail::reduce_nested_variadic
#else
#  define ABL_REDUCE_NESTED_NON_VARIADIC_IMPL \
  ::atria::xform::detail::reduce_nested_non_variadic
#endif

//!
// Similar to @a reduce, but does not unwrap `reduced` values.  This is
// useful when calling reduce recursively inside a transducer.
// @see take
//
template <typename ReducingFnT,
          typename StateT,
          typename InputRangeT>
auto reduce_nested(ReducingFnT&& step, StateT&& state, InputRangeT&& range)
  -> ABL_DECLTYPE_RETURN(
    ABL_REDUCE_NESTED_NON_VARIADIC_IMPL(
      std::forward<ReducingFnT>(step),
      std::forward<StateT>(state),
      std::forward<InputRangeT>(range)))

template <typename ReducingFnT,
          typename StateT,
          typename InputRangeT,
          typename ...InputRangeTs>
auto reduce_nested(ReducingFnT&& step, StateT&& state, InputRangeT&& range,
                   InputRangeTs&& ...ranges)
  -> ABL_DECLTYPE_RETURN(
    detail::reduce_nested_variadic(
      std::forward<ReducingFnT>(step),
      std::forward<StateT>(state),
      std::forward<InputRangeT>(range),
      std::forward<InputRangeTs>(ranges)...))

//!
// Similar to clojure.core/reduce.  Unlike `std::accumulate`, this
// reduces over a range (doesn't take to distinct iterators) and can
// reduce over several ranges at the same time.  It also supports
// early termination for transducers.
//
template <typename ReducingFnT,
          typename StateT,
          typename ...InputRangeTs>
auto reduce(ReducingFnT&& step, StateT&& state, InputRangeTs&& ...ranges)
  -> estd::decay_t<StateT>
{
  return state_complete(
    reduce_nested(
      std::forward<ReducingFnT>(step),
      std::forward<StateT>(state),
      std::forward<InputRangeTs>(ranges)...));
}

} // namespace xform
} // namespace atria
