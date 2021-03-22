//  Copyright (c) 2007-2017 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/reverse.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/concepts/concepts.hpp>
#include <hpx/iterator_support/traits/is_iterator.hpp>
#include <hpx/parallel/util/tagged_pair.hpp>

#include <hpx/executors/execution_policy.hpp>
#include <hpx/parallel/algorithms/copy.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/for_each.hpp>
#include <hpx/parallel/tagspec.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/projection_identity.hpp>
#include <hpx/parallel/util/result_types.hpp>
#include <hpx/parallel/util/zip_iterator.hpp>

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx { namespace parallel { inline namespace v1 {
    ///////////////////////////////////////////////////////////////////////////
    // reverse
    namespace detail {
        /// \cond NOINTERNAL
        template <typename Iter>
        struct reverse : public detail::algorithm<reverse<Iter>, Iter>
        {
            reverse()
              : reverse::algorithm("reverse")
            {
            }

            template <typename ExPolicy, typename BidirIter>
            static BidirIter sequential(
                ExPolicy, BidirIter first, BidirIter last)
            {
                std::reverse(first, last);
                return last;
            }

            template <typename ExPolicy, typename BidirIter>
            static typename util::detail::algorithm_result<ExPolicy,
                BidirIter>::type
            parallel(ExPolicy&& policy, BidirIter first, BidirIter last)
            {
                typedef std::reverse_iterator<BidirIter> destination_iterator;
                typedef hpx::util::zip_iterator<BidirIter, destination_iterator>
                    zip_iterator;
                typedef typename zip_iterator::reference reference;

                return util::detail::convert_to_result(
                    for_each_n<zip_iterator>().call(
                        std::forward<ExPolicy>(policy), std::false_type(),
                        hpx::util::make_zip_iterator(
                            first, destination_iterator(last)),
                        std::distance(first, last) / 2,
                        [](reference t) -> void {
                            using hpx::get;
                            std::swap(get<0>(t), get<1>(t));
                        },
                        util::projection_identity()),
                    [last](zip_iterator const&) -> BidirIter { return last; });
            }
        };
        /// \endcond
    }    // namespace detail

    /// Reverses the order of the elements in the range [first, last).
    /// Behaves as if applying std::iter_swap to every pair of iterators
    /// first+i, (last-i) - 1 for each non-negative i < (last-first)/2.
    ///
    /// \note   Complexity: Linear in the distance between \a first and \a last.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam BidirIter  The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     bidirectional iterator.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    ///
    /// The assignments in the parallel \a reverse algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a reverse algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a reverse algorithm returns a \a hpx::future<BidirIter>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a BidirIter otherwise.
    ///           It returns \a last.
    ///
    template <typename ExPolicy, typename BidirIter,
        HPX_CONCEPT_REQUIRES_(hpx::is_execution_policy<ExPolicy>::value&&
                hpx::traits::is_iterator<BidirIter>::value)>
    typename util::detail::algorithm_result<ExPolicy, BidirIter>::type reverse(
        ExPolicy&& policy, BidirIter first, BidirIter last)
    {
        static_assert(
            (hpx::traits::is_bidirectional_iterator<BidirIter>::value),
            "Requires at least bidirectional iterator.");

        return detail::reverse<BidirIter>().call(
            std::forward<ExPolicy>(policy), first, last);
    }

    ///////////////////////////////////////////////////////////////////////////
    // reverse_copy
    namespace detail {
        /// \cond NOINTERNAL

        // sequential reverse_copy
        template <typename BidirIt, typename OutIter>
        inline util::in_out_result<BidirIt, OutIter> sequential_reverse_copy(
            BidirIt first, BidirIt last, OutIter dest)
        {
            BidirIt iter = last;
            while (first != iter)
            {
                *dest++ = *--iter;
            }
            return util::in_out_result<BidirIt, OutIter>{last, dest};
        }

        template <typename IterPair>
        struct reverse_copy
          : public detail::algorithm<reverse_copy<IterPair>, IterPair>
        {
            reverse_copy()
              : reverse_copy::algorithm("reverse_copy")
            {
            }

            template <typename ExPolicy, typename BidirIter, typename OutIter>
            static util::in_out_result<BidirIter, OutIter> sequential(
                ExPolicy, BidirIter first, BidirIter last, OutIter dest_first)
            {
                return sequential_reverse_copy(first, last, dest_first);
            }

            template <typename ExPolicy, typename BidirIter, typename FwdIter>
            static typename util::detail::algorithm_result<ExPolicy,
                util::in_out_result<BidirIter, FwdIter>>::type
            parallel(ExPolicy&& policy, BidirIter first, BidirIter last,
                FwdIter dest_first)
            {
                typedef std::reverse_iterator<BidirIter> iterator;

                return util::detail::convert_to_result(
                    detail::copy<util::in_out_result<iterator, FwdIter>>().call(
                        std::forward<ExPolicy>(policy), std::false_type(),
                        iterator(last), iterator(first), dest_first),
                    [](util::in_out_result<iterator, FwdIter> const& p)
                        -> util::in_out_result<BidirIter, FwdIter> {
                        return util::in_out_result<BidirIter, FwdIter>{
                            p.in.base(), p.out};
                    });
            }
        };
        /// \endcond
    }    // namespace detail

    /// Copies the elements from the range [first, last) to another range
    /// beginning at dest_first in such a way that the elements in the new
    /// range are in reverse order.
    /// Behaves as if by executing the assignment
    /// *(dest_first + (last - first) - 1 - i) = *(first + i) once for each
    /// non-negative i < (last - first)
    /// If the source and destination ranges (that is, [first, last) and
    /// [dest_first, dest_first+(last-first)) respectively) overlap, the
    /// behavior is undefined.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam BidirIter   The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     bidirectional iterator.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest_first   Refers to the begin of the destination range.
    ///
    /// The assignments in the parallel \a reverse_copy algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a reverse_copy algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a reverse_copy algorithm returns a
    ///           \a hpx::future<tagged_pair<tag::in(BidirIter), tag::out(FwdIter)> >
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a tagged_pair<tag::in(BidirIter), tag::out(FwdIter)>
    ///           otherwise.
    ///           The \a copy algorithm returns the pair of the input iterator
    ///           forwarded to the first element after the last in the input
    ///           sequence and the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename ExPolicy, typename BidirIter, typename FwdIter,
        HPX_CONCEPT_REQUIRES_(hpx::traits::is_iterator<BidirIter>::value&&
                hpx::is_execution_policy<ExPolicy>::value&&
                    hpx::traits::is_iterator<FwdIter>::value)>
    typename util::detail::algorithm_result<ExPolicy,
        util::in_out_result<BidirIter, FwdIter>>::type
    reverse_copy(
        ExPolicy&& policy, BidirIter first, BidirIter last, FwdIter dest_first)
    {
        static_assert(
            (hpx::traits::is_bidirectional_iterator<BidirIter>::value),
            "Requires at least bidirectional iterator.");
        static_assert((hpx::traits::is_forward_iterator<FwdIter>::value),
            "Requires at least forward iterator.");

        return detail::reverse_copy<util::in_out_result<BidirIter, FwdIter>>()
            .call(std::forward<ExPolicy>(policy), first, last, dest_first);
    }
}}}    // namespace hpx::parallel::v1
