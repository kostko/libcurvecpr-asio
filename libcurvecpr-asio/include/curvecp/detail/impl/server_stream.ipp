/*
 * Copyright (C) 2014 Jernej Kos (jernej@kos.mx)
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef CURVECP_ASIO_DETAIL_IMPL_SERVER_STREAM_IPP
#define CURVECP_ASIO_DETAIL_IMPL_SERVER_STREAM_IPP

namespace curvecp {

namespace detail {

server_stream::server_stream(boost::shared_ptr<acceptor> acceptor,
                             boost::shared_ptr<session> session)
  : basic_stream(*session),
    acceptor_(acceptor),
    session_(session)
{
}

void server_stream::close()
{
  session_->close();
}

}

}

#endif
