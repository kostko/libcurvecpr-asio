/*
 * Copyright (C) 2014 Jernej Kos (jernej@kos.mx)
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef CURVECP_ASIO_DETAIL_ACCEPT_OP_HPP
#define CURVECP_ASIO_DETAIL_ACCEPT_OP_HPP

#include <boost/bind.hpp>

namespace curvecp {

namespace detail {

/**
 * Async accept operation processor.
 */
template <typename Acceptor, typename Stream, typename Handler>
class accept_op {
public:
  /**
   * Constructs an async accept operation.
   *
   * @param acceptor Acceptor reference
   * @param stream Target stream reference
   * @param handler Handler to call after accept completes
   */
  accept_op(Acceptor &acceptor, Stream &stream, Handler &handler)
    : acceptor_(acceptor),
      stream_(stream),
      handler_(BOOST_ASIO_MOVE_CAST(Handler)(handler)),
      finished_(false)
  {
  }

  /**
   * Executes the accept operation. If the operation needs to be retried
   * it is scheduled via the underlying acceptor.
   *
   * @param start Set to true for direct invocation by caller
   */
  void operator()(const boost::system::error_code &ec = boost::system::error_code(),
                  bool start = false)
  {
    if (!finished_) {
      if (!acceptor_.accept(stream_, ec_)) {
        acceptor_.async_pending_accept_wait(BOOST_ASIO_MOVE_CAST(accept_op)(*this));
        return;
      }

      finished_ = true;
      if (start) {
        // We are being called directly by the async operation, so need to
        // defer the invocation of the handler
        acceptor_.get_io_service().post(BOOST_ASIO_MOVE_CAST(accept_op)(*this));
        return;
      }
    }

    // Call accept handler
    handler_(ec_);
  }
private:
  /// Acceptor reference
  Acceptor &acceptor_;
  /// Target stream
  Stream &stream_;
  /// Handler to call after accept completes
  Handler handler_;
  /// Resulting error code
  boost::system::error_code ec_;
  /// Operation finished flag
  bool finished_;
};

}

}

#endif
