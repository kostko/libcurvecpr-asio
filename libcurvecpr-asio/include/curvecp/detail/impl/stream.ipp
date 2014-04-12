/*
 * Copyright (C) 2014 Jernej Kos (jernej@kos.mx)
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef CURVECP_ASIO_DETAIL_IMPL_STREAM_IPP
#define CURVECP_ASIO_DETAIL_IMPL_STREAM_IPP

#include <curvecp/detail/io.hpp>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/read.hpp>

#include <iomanip>

namespace curvecp {

namespace detail {

stream::stream(boost::asio::io_service &service)
  : socket_(service),
    session_(
      service,
      session::type::client,
      boost::bind(&stream::handle_upper_send, this, _1, _2)
    ),
    transmit_queue_maximum_(128),
    lower_recv_buffer_(65535),
    hello_timed_out_(service)
{
  struct curvecpr_client_cf client_cf;
  client_cf.ops.send = &stream::handle_send;
  client_cf.ops.recv = &stream::handle_recv;
  client_cf.ops.next_nonce = &stream::handle_next_nonce;
  client_cf.priv = this;

  // Initialize new client packet processor
  curvecpr_client_new(&client_, &client_cf);
}

void stream::set_local_extension(const std::string &extension)
{
  std::memset(client_.cf.my_extension, 0, sizeof(client_.cf.my_extension));
  std::memcpy(client_.cf.my_extension, extension.data(), sizeof(client_.cf.my_extension));
}

void stream::set_local_public_key(const std::string &publicKey)
{
  std::memset(client_.cf.my_global_pk, 0, sizeof(client_.cf.my_global_pk));
  std::memcpy(client_.cf.my_global_pk, publicKey.data(), sizeof(client_.cf.my_global_pk));
}

// TODO botan secure vector?
void stream::set_local_private_key(const std::string &privateKey)
{
  std::memset(client_.cf.my_global_sk, 0, sizeof(client_.cf.my_global_sk));
  std::memcpy(client_.cf.my_global_sk, privateKey.data(), sizeof(client_.cf.my_global_sk));
}

void stream::set_remote_extension(const std::string &extension)
{
  std::memset(client_.cf.their_extension, 0, sizeof(client_.cf.their_extension));
  std::memcpy(client_.cf.their_extension, extension.data(), sizeof(client_.cf.their_extension));
}

void stream::set_remote_public_key(const std::string &publicKey)
{
  std::memset(client_.cf.their_global_pk, 0, sizeof(client_.cf.their_global_pk));
  std::memcpy(client_.cf.their_global_pk, publicKey.data(), sizeof(client_.cf.their_global_pk));
}

void stream::set_remote_domain_name(const std::string &domain)
{
  std::memset(client_.cf.their_domain_name, 0, sizeof(client_.cf.their_domain_name));
  curvecpr_util_encode_domain_name(client_.cf.their_domain_name, domain.data());
}

template <typename NonceGenerator>
void stream::set_nonce_generator(NonceGenerator generator)
{
  nonce_generator_ = generator;
}

void stream::bind(const endpoint_type &endpoint)
{
  socket_.bind(endpoint);
}

void stream::connect(const endpoint_type &endpoint)
{
  socket_.connect(endpoint);
  socket_.async_receive(
    boost::asio::buffer(lower_recv_buffer_),
    session_.get_strand().wrap(boost::bind(&stream::handle_lower_read, this,
      boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred))
  );

  handle_hello_timeout(boost::system::error_code());
}

void stream::close()
{
  hello_timed_out_.cancel();
  session_.close();
}

template <typename Operation, typename Handler>
void stream::async_io_operation(const Operation &op, Handler &handler)
{
  io_op<stream, Operation, Handler>(session_, *this, op, handler)(boost::system::error_code(), true);
}

void stream::handle_hello_timeout(const boost::system::error_code &error)
{
  if (error)
    return;

  // Resend hello packet and restart the timer
  curvecpr_client_connected(&client_);

  hello_timed_out_.expires_from_now(boost::posix_time::seconds(1));
  hello_timed_out_.async_wait(boost::bind(&stream::handle_hello_timeout, this, _1));
}

void stream::transmit_pending()
{
  // Send the first item in the transmit queue
  socket_.async_send(
    boost::asio::buffer(&transmit_queue_.front()[0], transmit_queue_.front().size()),
    session_.get_strand().wrap(boost::bind(&stream::handle_lower_write, this,
      boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred))
  );
}

bool stream::handle_upper_send(const unsigned char *buffer, std::size_t length)
{
  return curvecpr_client_send(&client_, buffer, length) == 0;
}

void stream::handle_lower_write(const boost::system::error_code &error, std::size_t bytes)
{
  transmit_queue_.pop_front();
  if (error)
    return; // TODO error handling
  else if (!transmit_queue_.empty())
    transmit_pending();
}

void stream::handle_lower_read(const boost::system::error_code &error, std::size_t bytes)
{
  // Push received datagram into client
  if (curvecpr_client_recv(&client_, &lower_recv_buffer_[0], bytes) != 0)
    return;

  if (client_.negotiated != curvecpr_client::CURVECPR_CLIENT_PENDING) {
    hello_timed_out_.cancel();
    session_.start();
  }

  socket_.async_receive(
    boost::asio::buffer(lower_recv_buffer_),
    session_.get_strand().wrap(boost::bind(&stream::handle_lower_read, this,
      boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred))
  );
}

int stream::handle_send(struct curvecpr_client *client,
                        const unsigned char *buf,
                        size_t num)
{
  stream *self = static_cast<stream*>(client->cf.priv);

  if (self->transmit_queue_.size() >= self->transmit_queue_maximum_)
    return -1;

  std::vector<unsigned char> buffer(num);
  std::memcpy(&buffer[0], buf, num);
  self->transmit_queue_.push_back(buffer);

  // If this is the only item in the queue, transmit immediately
  if (self->transmit_queue_.size() == 1)
    self->transmit_pending();

  return 0;
}

int stream::handle_recv(struct curvecpr_client *client,
                        const unsigned char *buf,
                        size_t num)
{
  stream *self = static_cast<stream*>(client->cf.priv);
  return self->session_.lower_receive(buf, num);
}

int stream::handle_next_nonce(struct curvecpr_client *client,
                              unsigned char *destination,
                              size_t num)
{
  stream *self = static_cast<stream*>(client->cf.priv);
  if (!self->nonce_generator_)
    return -1;

  self->nonce_generator_(destination, num);
  return 0;
}

}

}

#endif
