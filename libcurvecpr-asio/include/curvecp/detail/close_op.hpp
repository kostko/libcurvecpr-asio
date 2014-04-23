/*
 * Copyright (C) 2014 Jernej Kos (jernej@kos.mx)
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef CURVECP_ASIO_DETAIL_CLOSE_OP_HPP
#define CURVECP_ASIO_DETAIL_CLOSE_OP_HPP

#include <curvecp/detail/session.hpp>

namespace curvecp {

namespace detail {

/**
 * Implementation of an async close operation.
 */
class close_op {
public:
  /**
   * Constructs an async close operation.
   */
  close_op()
  {
  }

  /**
   * Executes the read operation.
   *
   * @param session Internal CurveCP session reference
   * @return Whether the operation should be retried
   */
  session::want operator()(session &session,
                           boost::system::error_code&,
                           std::size_t&) const
  {
    return session.close() ? session::want::nothing : session::want::close;
  }

  /**
   * Calls the handler for this operation.
   *
   * @param handler Handler reference
   * @param ec Error code
   * @param bytes_transferred Number of bytes transferred
   */
  template <typename Handler>
  void call_handler(Handler &handler,
                    const boost::system::error_code&,
                    const std::size_t&) const
  {
    handler();
  }
};

}

}

#endif
