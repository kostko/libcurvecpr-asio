/*
 * Copyright (C) 2014 Jernej Kos (jernej@kos.mx)
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef CURVECP_ASIO_DETAIL_READ_OP_HPP
#define CURVECP_ASIO_DETAIL_READ_OP_HPP

#include <curvecp/detail/session.hpp>

namespace curvecp {

namespace detail {

/**
 * Implementation of an async read operation.
 */
template <typename MutableBufferSequence>
class read_op {
public:
  /**
   * Constructs an async read operation.
   *
   * @param buffers A mutable buffer sequence to write to
   */
  read_op(const MutableBufferSequence& buffers)
    : buffers_(buffers)
  {
  }

  /**
   * Executes the read operation.
   *
   * @param session Internal CurveCP session reference
   * @param ec Output error code
   * @param bytes_transferred Output number of bytes transferred
   * @return Whether the operation should be retried
   */
  session::want operator()(session &session,
                           boost::system::error_code &ec,
                           std::size_t &bytes_transferred) const
  {
    boost::asio::mutable_buffer buffer =
      boost::asio::detail::buffer_sequence_adapter<boost::asio::mutable_buffer,
        MutableBufferSequence>::first(buffers_);

    return session.read(buffer, ec, bytes_transferred) ? session::want::nothing : session::want::read;
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
                    const boost::system::error_code &ec,
                    const std::size_t &bytes_transferred) const
  {
    handler(ec, bytes_transferred);
  }
private:
  MutableBufferSequence buffers_;
};

}

}

#endif
