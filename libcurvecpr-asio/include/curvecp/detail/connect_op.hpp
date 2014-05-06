/*
 * Copyright (C) 2014 Jernej Kos (jernej@kos.mx)
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef CURVECP_ASIO_DETAIL_CONNECT_OP_HPP
#define CURVECP_ASIO_DETAIL_CONNECT_OP_HPP

#include <boost/bind.hpp>

namespace curvecp {

namespace detail {

/**
 * Async accept operation processor.
 */
template <typename Stream, typename Handler>
class connect_op {
public:
  /**
   * Constructs an async connect operation.
   *
   * @param stream Target stream reference
   * @param handler Handler to call after accept completes
   */
  connect_op(const typename Stream::endpoint_type &endpoint, Stream &stream, Handler &handler)
    : endpoint_(endpoint),
      stream_(stream),
      handler_(BOOST_ASIO_MOVE_CAST(Handler)(handler)),
      finished_(false)
  {
  }

  /**
   * Executes the connect operation. If the operation needs to be retried
   * it is scheduled via the underlying stream.
   *
   * @param start Set to true for direct invocation by caller
   */
  void operator()(const boost::system::error_code &ec = boost::system::error_code(),
                  bool start = false)
  {
    if (!finished_) {
      if (!stream_.connect(endpoint_, ec_)) {
        stream_.async_pending_connect_wait(BOOST_ASIO_MOVE_CAST(connect_op)(*this));
        return;
      }

      finished_ = true;
      if (start) {
        // We are being called directly by the async operation, so need to
        // defer the invocation of the handler
        stream_.get_io_service().post(BOOST_ASIO_MOVE_CAST(connect_op)(*this));
        return;
      }
    }

    // Call connect handler
    handler_(ec_);
  }
private:
  /// Endpoint to connect to
  typename Stream::endpoint_type endpoint_;
  /// Stream
  Stream &stream_;
  /// Handler to call after connect completes
  Handler handler_;
  /// Resulting error code
  boost::system::error_code ec_;
  /// Operation finished flag
  bool finished_;
};

}

}

#endif
