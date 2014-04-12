/*
 * Copyright (C) 2014 Jernej Kos (jernej@kos.mx)
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef CURVECP_ASIO_DETAIL_IO_HPP
#define CURVECP_ASIO_DETAIL_IO_HPP

#include <curvecp/detail/session.hpp>

#include <boost/bind.hpp>

namespace curvecp {

namespace detail {

/**
 * Async IO operation processor.
 */
template <typename Stream, typename Operation, typename Handler>
class io_op {
public:
  /**
   * Constructs an async IO operation.
   *
   * @param session Internal CurveCP session reference
   * @param stream Stream reference
   * @param op IO operation to perform
   * @param handler Handler to call after operation completes
   */
  io_op(session &session, Stream &stream, const Operation &op, Handler &handler)
    : stream_(stream),
      op_(op),
      handler_(BOOST_ASIO_MOVE_CAST(Handler)(handler)),
      session_(session),
      finished_(false)
  {
  }

  /**
   * Executes the IO operation. If the operation needs to be retried
   * it is scheduled via the underlying stream.
   *
   * @param start Set to true for direct invocation by caller
   */
  void operator()(const boost::system::error_code &ec = boost::system::error_code(),
                  bool start = false)
  {
    if (!finished_) {
      session::want result = op_(session_, ec_, bytes_transferred_);
      if (result != session::want::nothing) {
        session_.async_pending_wait(result, BOOST_ASIO_MOVE_CAST(io_op)(*this));
        return;
      }

      finished_ = true;
      if (start) {
        // We are being called directly by the async operation, so need to
        // defer the invocation of th handler
        stream_.get_io_service().post(BOOST_ASIO_MOVE_CAST(io_op)(*this));
        return;
      }
    }

    op_.call_handler(handler_, ec_, bytes_transferred_);
  }
private:
  /// Stream reference
  Stream &stream_;
  /// IO operation to perform
  Operation op_;
  /// Handler to call after operation completes
  Handler handler_;
  /// Internal CurveCP session reference
  session &session_;
  /// Resulting error code
  boost::system::error_code ec_;
  /// Resulting number of bytes transferred
  std::size_t bytes_transferred_;
  /// Operation finished flag
  bool finished_;
};

}

}

#endif
