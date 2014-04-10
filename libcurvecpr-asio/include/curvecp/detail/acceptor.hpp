/*
 * Copyright (C) 2014 Jernej Kos (jernej@kos.mx)
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef CURVECP_ASIO_DETAIL_ACCEPTOR_HPP
#define CURVECP_ASIO_DETAIL_ACCEPTOR_HPP

#include <curvecp/detail/session.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/asio/ip/udp.hpp>
#include <unordered_map>

namespace curvecp {

namespace detail {

class acceptor {
public:
  // TODO: should handle put/get session
private:
  std::unordered_map<std::string, boost::shared_ptr<session>> sessions_;
  boost::asio::ip::udp::socket socket_;
};

}

}

#endif
