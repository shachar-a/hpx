////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#if !defined(HPX_0C9D09E0_725D_4FA6_A879_8226DE97C6B9)
#define HPX_0C9D09E0_725D_4FA6_A879_8226DE97C6B9

#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <hpx/hpx_fwd.hpp>
#include <hpx/util/io_service_pool.hpp>
#include <hpx/util/connection_cache.hpp>
#include <hpx/runtime/naming/address.hpp>

namespace hpx { namespace agas
{

struct HPX_EXPORT big_boot_barrier : boost::noncopyable
{
  private:
    parcelset::parcelport& pp;
    util::connection_cache<parcelset::parcelport_connection>& connection_cache_;
    util::io_service_pool& io_service_pool_;

    const router_mode router_type;
    const runtime_mode runtime_type;
    const naming::address bootstrap_agas;

    boost::condition_variable cond;
    boost::mutex mtx;
    bool connected;

    void spin();

  public:
    big_boot_barrier(
        parcelset::parcelport& pp_ 
      , util::runtime_configuration const& ini_
      , runtime_mode runtime_type_
    );

    void apply(
        actions::base_action* act 
      , naming::address const& addr
    );

    void wait();

    void notify();
};

HPX_EXPORT void create_big_boot_barrier(
    parcelset::parcelport& pp_ 
  , util::runtime_configuration const& ini_
  , runtime_mode runtime_type_
);

HPX_EXPORT big_boot_barrier& get_big_boot_barrier();

}}

#endif // HPX_0C9D09E0_725D_4FA6_A879_8226DE97C6B9

