/*
 * Copyright (C) 2014 Jernej Kos (jernej@kos.mx)
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef CURVECP_ASIO_ACCEPTOR_HPP
#define CURVECP_ASIO_ACCEPTOR_HPP

#include <curvecp/detail/acceptor.hpp>
#include <curvecp/stream.hpp>

namespace curvecp {

/**
 * CurveCP server acceptor.
 */
class acceptor {
public:
  acceptor();

  template <typename AcceptHandler>
  void async_accept(stream &peer, AcceptHandler handler)
  {
    // TODO
  }
private:
  /// Private acceptor implementation
  detail::acceptor impl_;
};

}

#endif
