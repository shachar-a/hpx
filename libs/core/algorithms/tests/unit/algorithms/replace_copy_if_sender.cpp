//  Copyright (c) 2014-2017 Hartmut Kaiser
//  Copyright (c)      2021 Giannis Gonidelis
//  Copyright (c)      2024 Tobias Wukovitsch
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
struct equal_f
{
    equal_f(std::size_t val)
      : val_(val)
    {
    }

    bool operator()(std::size_t lhs) const
    {
        return lhs == val_;
    }

    std::size_t val_;
};

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_replace_copy_if_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d1(c.size());
    std::vector<std::size_t> d2(c.size());    //-V656

    std::iota(std::begin(c), std::end(c), std::rand());

    std::size_t idx = std::rand() % c.size();    //-V104

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    tt::sync_wait(ex::just(iterator(std::begin(c)), iterator(std::end(c)),
                      std::begin(d1), equal_f(c[idx]), c[idx] + 1) |
        hpx::replace_copy_if(ex_policy.on(exec)));

    std::replace_copy_if(std::begin(c), std::end(c), std::begin(d2),
        equal_f(c[idx]), c[idx] + 1);

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(d1), std::end(d1), std::begin(d2),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d1.size());
}

template <typename IteratorTag>
void replace_copy_if_sender_test()
{
    using namespace hpx::execution;
    test_replace_copy_if_sender(hpx::launch::sync, seq(task), IteratorTag());
    test_replace_copy_if_sender(hpx::launch::sync, unseq(task), IteratorTag());

    test_replace_copy_if_sender(hpx::launch::async, par(task), IteratorTag());
    test_replace_copy_if_sender(
        hpx::launch::async, par_unseq(task), IteratorTag());
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    replace_copy_if_sender_test<std::forward_iterator_tag>();
    replace_copy_if_sender_test<std::random_access_iterator_tag>();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // add command line option which controls the random number generator seed
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");

    // By default this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}