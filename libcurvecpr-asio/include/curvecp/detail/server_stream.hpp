/*
 * Copyright (C) 2014 Jernej Kos (jernej@kos.mx)
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef CURVECP_ASIO_DETAIL_SERVER_STREAM_HPP
#define CURVECP_ASIO_DETAIL_SERVER_STREAM_HPP

#include <curvecp/detail/basic_stream.hpp>

#include <boost/shared_ptr.hpp>

namespace curvecp {

namespace detail {

/**
 * Internal CurveCP stream handling implementation.
 */
class server_stream : public basic_stream {
public:
  /**
   * Constructs an internal CurveCP server stream implementation.
   *
   * @param acceptor Acceptor instance
   * @param session Session instance
   */
  server_stream(boost::shared_ptr<acceptor> acceptor,
                boost::shared_ptr<session> session);

  /**
   * Returns the ASIO IO service associated with this stream.
   */
  boost::asio::io_service &get_io_service() { return acceptor_->get_io_service(); }

  /**
   * Closes the stream.
   */
  void close();
protected:
  boost::shared_ptr<acceptor> acceptor_;
  boost::shared_ptr<session> session_;
};

}

}

#include <curvecp/detail/impl/server_stream.ipp>

#endif
