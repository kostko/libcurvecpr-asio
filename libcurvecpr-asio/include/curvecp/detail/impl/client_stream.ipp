/*
 * Copyright (C) 2014 Jernej Kos (jernej@kos.mx)
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef CURVECP_ASIO_DETAIL_IMPL_CLIENT_STREAM_IPP
#define CURVECP_ASIO_DETAIL_IMPL_CLIENT_STREAM_IPP

#include <curvecp/detail/io.hpp>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/read.hpp>

namespace curvecp {

namespace detail {

client_stream::client_stream(boost::asio::io_service &service)
  : basic_stream(service, session_),
    socket_(service),
    session_(service, session::type::client),
    lower_recv_buffer_(65535),
    hello_timed_out_(service),
    hello_retries_(0)
{
  session_.set_lower_send_handler(boost::bind(&client_stream::handle_upper_send, this, _1, _2));
  session_.set_close_handler([this]() {
    hello_timed_out_.cancel();
    socket_.close();
  });

  struct curvecpr_client_cf client_cf;
  client_cf.ops.send = &client_stream::handle_send;
  client_cf.ops.recv = &client_stream::handle_recv;
  client_cf.ops.next_nonce = &client_stream::handle_next_nonce;
  client_cf.priv = this;

  // Initialize new client packet processor
  curvecpr_client_new(&client_, &client_cf);
}

void client_stream::set_local_extension(const std::string &extension)
{
  std::memset(client_.cf.my_extension, 0, sizeof(client_.cf.my_extension));
  std::memcpy(client_.cf.my_extension, extension.data(), sizeof(client_.cf.my_extension));
}

void client_stream::set_local_public_key(const std::string &publicKey)
{
  std::memset(client_.cf.my_global_pk, 0, sizeof(client_.cf.my_global_pk));
  std::memcpy(client_.cf.my_global_pk, publicKey.data(), sizeof(client_.cf.my_global_pk));
}

// TODO botan secure vector?
void client_stream::set_local_private_key(const std::string &privateKey)
{
  std::memset(client_.cf.my_global_sk, 0, sizeof(client_.cf.my_global_sk));
  std::memcpy(client_.cf.my_global_sk, privateKey.data(), sizeof(client_.cf.my_global_sk));
}

void client_stream::set_remote_extension(const std::string &extension)
{
  std::memset(client_.cf.their_extension, 0, sizeof(client_.cf.their_extension));
  std::memcpy(client_.cf.their_extension, extension.data(), sizeof(client_.cf.their_extension));
}

void client_stream::set_remote_public_key(const std::string &publicKey)
{
  std::memset(client_.cf.their_global_pk, 0, sizeof(client_.cf.their_global_pk));
  std::memcpy(client_.cf.their_global_pk, publicKey.data(), sizeof(client_.cf.their_global_pk));
}

void client_stream::set_remote_domain_name(const std::string &domain)
{
  std::memset(client_.cf.their_domain_name, 0, sizeof(client_.cf.their_domain_name));
  curvecpr_util_encode_domain_name(client_.cf.their_domain_name, domain.data());
}

void client_stream::bind(const endpoint_type &endpoint)
{
  socket_.open(endpoint.protocol());
  socket_.bind(endpoint);
}

bool client_stream::connect(const endpoint_type &endpoint, boost::system::error_code &ec)
{
  if (session_.is_running()) {
    ec = boost::system::error_code();
    return true;
  } else {
    if (hello_retries_ < 0) {
      hello_retries_ = 0;
      ec = boost::system::error_code(boost::asio::error::connection_refused);
      return true;
    }

    hello_retries_ = 0;
    socket_.connect(endpoint);
    socket_.async_receive(
      boost::asio::buffer(lower_recv_buffer_),
      session_.get_strand().wrap(boost::bind(&client_stream::handle_lower_read, this,
        boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred))
    );

    handle_hello_timeout(boost::system::error_code());
    return false;
  }
}

void client_stream::handle_hello_timeout(const boost::system::error_code &error)
{
  if (error)
    return;

  if (++hello_retries_ > 5) {
    hello_retries_ = -1;
    pending_ready_connect_.cancel();
    socket_.close();
    return;
  }

  // Resend hello packet and restart the timer
  curvecpr_client_connected(&client_);

  hello_timed_out_.expires_from_now(boost::posix_time::seconds(1));
  hello_timed_out_.async_wait(session_.get_strand().wrap(
    boost::bind(&client_stream::handle_hello_timeout, this, _1)));
}

void client_stream::handle_upper_send(const unsigned char *buffer, std::size_t length)
{
  curvecpr_client_send(&client_, buffer, length);
}

void client_stream::handle_lower_read(const boost::system::error_code &error, std::size_t bytes)
{
  if (error == boost::asio::error::operation_aborted || error == boost::asio::error::bad_descriptor)
    return;

  // Push received datagram into client
  if (curvecpr_client_recv(&client_, &lower_recv_buffer_[0], bytes) == 0) {
    if (client_.negotiated != curvecpr_client::CURVECPR_CLIENT_PENDING) {
      hello_retries_ = 0;
      hello_timed_out_.cancel();
      session_.start();
      pending_ready_connect_.cancel();
    }
  }

  socket_.async_receive(
    boost::asio::buffer(lower_recv_buffer_),
    session_.get_strand().wrap(boost::bind(&client_stream::handle_lower_read, this,
      boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred))
  );
}

int client_stream::handle_send(struct curvecpr_client *client,
                               const unsigned char *buf,
                               size_t num)
{
  client_stream *self = static_cast<client_stream*>(client->cf.priv);

  boost::shared_ptr<std::vector<unsigned char>> buffer(
    boost::make_shared<std::vector<unsigned char>>(num));
  std::memcpy(&(*buffer)[0], buf, num);

  // Transmit data
  self->socket_.async_send(
    boost::asio::buffer(&(*buffer)[0], num),
    self->session_.get_strand().wrap([buffer](const boost::system::error_code&, std::size_t) {})
  );

  return 0;
}

int client_stream::handle_recv(struct curvecpr_client *client,
                               const unsigned char *buf,
                               size_t num)
{
  client_stream *self = static_cast<client_stream*>(client->cf.priv);
  return self->session_.lower_receive(buf, num);
}

int client_stream::handle_next_nonce(struct curvecpr_client *client,
                                     unsigned char *destination,
                                     size_t num)
{
  client_stream *self = static_cast<client_stream*>(client->cf.priv);
  if (!self->nonce_generator_)
    return -1;

  self->nonce_generator_(destination, num);
  return 0;
}

}

}

#endif
