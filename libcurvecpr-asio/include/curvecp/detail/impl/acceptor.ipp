/*
 * Copyright (C) 2014 Jernej Kos (jernej@kos.mx)
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef CURVECP_ASIO_DETAIL_IMPL_ACCEPTOR_IPP
#define CURVECP_ASIO_DETAIL_IMPL_ACCEPTOR_IPP

#include <curvecp/detail/server_stream.hpp>

#include <boost/make_shared.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/ip/udp.hpp>

namespace curvecp {

namespace detail {

acceptor::acceptor(boost::asio::io_service &service)
  : strand_(service),
    socket_(service),
    maximum_pending_sessions_(16),
    transmit_queue_maximum_(512),
    lower_recv_buffer_(65535),
    pending_ready_accept_(service)
{
  pending_ready_accept_.expires_at(boost::posix_time::pos_infin);

  struct curvecpr_server_cf server_cf;
  server_cf.ops.put_session = &acceptor::handle_put_session;
  server_cf.ops.get_session = &acceptor::handle_get_session;
  server_cf.ops.send = &acceptor::handle_send;
  server_cf.ops.recv = &acceptor::handle_recv;
  server_cf.ops.next_nonce = &acceptor::handle_next_nonce;
  server_cf.priv = this;

  // Initialize new server packet processor
  curvecpr_server_new(&server_, &server_cf);
}

void acceptor::set_local_extension(const std::string &extension)
{
  std::memset(server_.cf.my_extension, 0, sizeof(server_.cf.my_extension));
  std::memcpy(server_.cf.my_extension, extension.data(), sizeof(server_.cf.my_extension));
}

void acceptor::set_local_public_key(const std::string &publicKey)
{
  std::memset(server_.cf.my_global_pk, 0, sizeof(server_.cf.my_global_pk));
  std::memcpy(server_.cf.my_global_pk, publicKey.data(), sizeof(server_.cf.my_global_pk));
}

void acceptor::set_local_private_key(const std::string &privateKey)
{
  std::memset(server_.cf.my_global_sk, 0, sizeof(server_.cf.my_global_sk));
  std::memcpy(server_.cf.my_global_sk, privateKey.data(), sizeof(server_.cf.my_global_sk));
}

void acceptor::bind(const typename detail::basic_stream::endpoint_type &endpoint)
{
  socket_.open(endpoint.protocol());
  socket_.bind(endpoint);
}

void acceptor::listen()
{
  socket_.async_receive_from(
    boost::asio::buffer(lower_recv_buffer_),
    lower_recv_endpoint_,
    strand_.wrap(boost::bind(&acceptor::handle_lower_read, this,
      boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred))
  );
}

bool acceptor::accept(curvecp::stream &stream, boost::system::error_code &error)
{
  error = boost::system::error_code();
  if (pending_sessions_.empty())
    return false;

  boost::shared_ptr<session> sp = pending_sessions_.front();
  pending_sessions_.pop_front();
  stream.stream_ = boost::make_shared<detail::server_stream>(shared_from_this(), sp);
  sp->start();
  return true;
}

template <typename Handler>
void acceptor::async_pending_accept_wait(BOOST_ASIO_MOVE_ARG(Handler) handler)
{
  pending_ready_accept_.async_wait(strand_.wrap(handler));
}

void acceptor::transmit_pending()
{
  // Send the first item in the transmit queue
  socket_.async_send_to(
    boost::asio::buffer(&transmit_queue_.front().data[0], transmit_queue_.front().data.size()),
    transmit_queue_.front().endpoint,
    strand_.wrap(boost::bind(&acceptor::handle_lower_write, this,
      boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred))
  );
}

void acceptor::handle_lower_write(const boost::system::error_code &error, std::size_t bytes)
{
  transmit_queue_.pop_front();
  if (error)
    return; // TODO error handling
  else if (!transmit_queue_.empty())
    transmit_pending();
}

void acceptor::handle_lower_read(const boost::system::error_code &error, std::size_t bytes)
{
  // Push received datagram into server
  curvecpr_session *s = nullptr;
  if (curvecpr_server_recv(&server_, nullptr, &lower_recv_buffer_[0], bytes, &s) == 0) {
    if (s) {
      // Update client endpoint
      session *sp = static_cast<session*>(s->priv);
      sp->set_endpoint(lower_recv_endpoint_);
    }
  }

  socket_.async_receive_from(
    boost::asio::buffer(lower_recv_buffer_),
    lower_recv_endpoint_,
    strand_.wrap(boost::bind(&acceptor::handle_lower_read, this,
      boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred))
  );
}

void acceptor::handle_upper_send(boost::shared_ptr<session> session,
                                 const unsigned char *buffer,
                                 std::size_t length)
{
  curvecpr_server_send(&server_, &session->session_, nullptr, buffer, length);
}

void acceptor::handle_session_close(const std::string &sessionKey)
{
  sessions_.erase(sessionKey);
}

int acceptor::handle_put_session(struct curvecpr_server *server,
                                 const struct curvecpr_session *s,
                                 void *priv,
                                 struct curvecpr_session **s_stored)
{
  acceptor *self = static_cast<acceptor*>(server->cf.priv);
  if (self->pending_sessions_.size() >= self->maximum_pending_sessions_)
    return 1;

  // Create a new session descriptor
  boost::shared_ptr<session> sp = boost::make_shared<session>(self->get_io_service(),
    session::type::server);
  sp->set_lower_send_handler(self->strand_.wrap(boost::bind(&acceptor::handle_upper_send, self, sp, _1, _2)));
  sp->session_ = *s;
  sp->session_.priv = sp.get();
  sp->set_endpoint(self->lower_recv_endpoint_);
  // Store session under its public key
  std::string sessionKey((const char*) sp->session_.their_session_pk, 32);
  self->sessions_.insert({{ sessionKey, sp }});
  sp->set_close_handler(self->strand_.wrap(boost::bind(&acceptor::handle_session_close, self, sessionKey)));
  // Put session parameters into the pending session queue
  self->pending_sessions_.push_back(sp);
  // Notify waiting acceptors
  self->pending_ready_accept_.cancel();

  if (s_stored)
    *s_stored = &sp->session_;

  return 0;
}

int acceptor::handle_get_session(struct curvecpr_server *server,
                                 const unsigned char their_session_pk[32],
                                 struct curvecpr_session **s_stored)
{
  acceptor *self = static_cast<acceptor*>(server->cf.priv);

  // Lookup session descriptor
  auto it = self->sessions_.find(std::string((const char*) their_session_pk, 32));
  if (it == self->sessions_.end())
    return 1;

  if (s_stored)
    *s_stored = &(*it).second->session_;

  return 0;
}

int acceptor::handle_send(struct curvecpr_server *server,
                          struct curvecpr_session *s,
                          void *priv,
                          const unsigned char *buf,
                          size_t num)
{
  acceptor *self = static_cast<acceptor*>(server->cf.priv);
  if (self->transmit_queue_.size() >= self->transmit_queue_maximum_)
    return -1;

  boost::asio::ip::udp::endpoint endpoint;
  if (!s->priv) {
    // We are being called while receiving a Hello packet from client,
    // so we can use the endpoint of the last received datagram
    endpoint = self->lower_recv_endpoint_;
  } else {
    // Use last known endpoint of a session
    endpoint = static_cast<session*>(s->priv)->get_endpoint();
  }

  transmit_datagram datagram;
  datagram.endpoint = endpoint;
  datagram.data.resize(num);
  std::memcpy(&datagram.data[0], buf, num);
  self->transmit_queue_.push_back(std::move(datagram));

  // If this is the only item in the queue, transmit immediately
  if (self->transmit_queue_.size() == 1)
    self->transmit_pending();

  return 0;
}

int acceptor::handle_recv(struct curvecpr_server *server,
                          struct curvecpr_session *s,
                          void *priv,
                          const unsigned char *buf,
                          size_t num)
{
  session *sp = static_cast<session*>(s->priv);
  return sp->lower_receive(buf, num);
}

int acceptor::handle_next_nonce(struct curvecpr_server *server,
                                unsigned char *destination,
                                size_t num)
{
  acceptor *self = static_cast<acceptor*>(server->cf.priv);
  if (!self->nonce_generator_)
    return -1;

  self->nonce_generator_(destination, num);
  return 0;
}

}

}

#endif
