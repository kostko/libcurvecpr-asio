/*
 * Copyright (C) 2014 Jernej Kos (jernej@kos.mx)
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef CURVECP_ASIO_DETAIL_IMPL_SESSION_IPP
#define CURVECP_ASIO_DETAIL_IMPL_SESSION_IPP

#include <boost/bind.hpp>
#include <boost/asio/error.hpp>

namespace curvecp {

namespace detail {

#define RECVMARKQ_ELEMENT_NONE 0
#define RECVMARKQ_ELEMENT_DISTRIBUTED (1 << 0)
#define RECVMARKQ_ELEMENT_ACKNOWLEDGED (1 << 1)
#define RECVMARKQ_ELEMENT_DONE (RECVMARKQ_ELEMENT_DISTRIBUTED | RECVMARKQ_ELEMENT_ACKNOWLEDGED)

session::session(boost::asio::io_service &service,
                 type session_type)
  : strand_(service),
    pending_maximum_(65536),
    sendmarkq_maximum_(1024),
    recvmarkq_maximum_(1024),
    pending_eof_(false),
    pending_used_(0),
    pending_current_(0),
    pending_next_(0),
    sendq_head_exists_(false),
    recvmarkq_distributed_(0),
    recvmarkq_read_offset_(0),
    send_queue_timer_(service),
    pending_ready_read_(service),
    pending_ready_write_(service),
    running_(false)
{
  pending_ready_read_.expires_at(boost::posix_time::pos_infin);
  pending_ready_write_.expires_at(boost::posix_time::pos_infin);

  // Configure curvecpr messager handlers
  struct curvecpr_messager_cf messager_cf = {
    .ops = {
      .sendq_head = &session::handle_sendq_head,
      .sendq_move_to_sendmarkq = &session::handle_sendq_move_to_sendmarkq,
      .sendq_is_empty = &session::handle_sendq_is_empty,

      .sendmarkq_head = &session::handle_sendmarkq_head,
      .sendmarkq_get = &session::handle_sendmarkq_get,
      .sendmarkq_remove_range = &session::handle_sendmarkq_remove_range,
      .sendmarkq_is_full = &session::handle_sendmarkq_is_full,

      .recvmarkq_put = &session::handle_recvmarkq_put,
      .recvmarkq_get_nth_unacknowledged = &session::handle_recvmarkq_get_nth_unacknowledged,
      .recvmarkq_is_empty = &session::handle_recvmarkq_is_empty,
      .recvmarkq_remove_range = &session::handle_recvmarkq_remove_range,

      .send = &session::handle_send,

      .put_next_timeout = nullptr
    },
    .priv = this
  };

  // Initialize the curvecpr messager
  curvecpr_messager_new(&messager_, &messager_cf, session_type == session::type::client ? 1 : 0);
}

template <typename Handler>
void session::async_pending_wait(session::want what, BOOST_ASIO_MOVE_ARG(Handler) handler)
{
  switch (what) {
    case session::want::read: pending_ready_read_.async_wait(strand_.wrap(handler)); break;
    case session::want::write: pending_ready_write_.async_wait(strand_.wrap(handler)); break;
    default: break;
  }
}

void session::start()
{
  running_ = true;
  handle_process_send_queue(boost::system::error_code());
}

void session::handle_process_send_queue(const boost::system::error_code &error)
{
  if (error)
    return;

  curvecpr_messager_process_sendq(&messager_);
  reschedule_process_send_queue();
}

void session::reschedule_process_send_queue()
{
  send_queue_timer_.expires_from_now(
    boost::posix_time::microseconds(curvecpr_messager_next_timeout(&messager_) / 1000)
  );
  send_queue_timer_.async_wait(strand_.wrap(boost::bind(&session::handle_process_send_queue, this, _1)));
}

bool session::is_finished() const
{
  return messager_.my_final && messager_.their_final;
}

void session::finish()
{
  if (pending_eof_)
    return;

  pending_eof_ = true;
  pending_ready_read_.cancel();
  pending_ready_write_.cancel();
}

void session::close()
{
  // Finish the session first
  pending_eof_ = true;
  pending_ready_read_.cancel();
  pending_ready_write_.cancel();
  send_queue_timer_.cancel();

  // Transmit EOF immediately
  curvecpr_messager_process_sendq(&messager_);

  // Reinitialize the session
  pending_eof_ = false;
  pending_used_ = 0;
  pending_current_ = 0;
  pending_next_ = 0;
  sendq_head_exists_ = false;
  recvmarkq_distributed_ = 0;
  recvmarkq_read_offset_ = 0;
  running_ = false;

  for (curvecpr_block *b : sendmarkq_)
    delete b;
  for (curvecpr_block_status *b : recvmarkq_)
    delete b;

  sendmarkq_.clear();
  recvmarkq_.clear();

  if (close_handler_)
    close_handler_();
}

int session::lower_receive(const unsigned char *buf, size_t num)
{
  return curvecpr_messager_recv(&messager_, buf, num);
}

bool session::read(const boost::asio::mutable_buffer &data,
                   boost::system::error_code &ec,
                   std::size_t &bytes_transferred)
{
  bytes_transferred = 0;
  ec = boost::system::error_code();

  if (boost::asio::buffer_size(data) == 0) {
    recvmarkq_read_offset_ = 0;
    return true;
  }

  // Check if there are enough sequential blocks available in the buffer
  size_t buffer_length = boost::asio::buffer_size(data);
  unsigned char *buffer = boost::asio::buffer_cast<unsigned char*>(data) + recvmarkq_read_offset_;

  for (auto it = recvmarkq_.begin(); it != recvmarkq_.end();) {
    auto jt = it;
    ++it;

    if ((*jt)->block.offset <= recvmarkq_distributed_) {
      if ((*jt)->block.data_len > 0 && (*jt)->block.offset + (*jt)->block.data_len > recvmarkq_distributed_) {
        std::uint64_t idx = recvmarkq_distributed_ - (*jt)->block.offset;
        size_t len = (*jt)->block.data_len - idx;

        bool should_break = false;
        if (len > buffer_length - recvmarkq_read_offset_) {
          // This block has more data than we need, so we can't yet mark this block as distributed
          len = buffer_length - recvmarkq_read_offset_;
          should_break = true;
        }

        std::memcpy(buffer, (*jt)->block.data + idx, len);
        recvmarkq_distributed_ += len;
        recvmarkq_read_offset_ += len;
        buffer += len;

        if (should_break)
          break;
      }

      if ((*jt)->block.eof != CURVECPR_BLOCK_STREAM)
        pending_eof_ = true;

      (*jt)->status |= RECVMARKQ_ELEMENT_DISTRIBUTED;

      // If this element has been acknowledged and distributed, remove it
      if ((*jt)->status == RECVMARKQ_ELEMENT_DONE) {
        delete *jt;
        recvmarkq_.erase(jt);
      }
    } else {
      // Since the blocks are sorted, nothing else will match after this
      break;
    }
  }

  if (recvmarkq_read_offset_ == buffer_length || pending_eof_) {
    // Read is complete
    bytes_transferred = recvmarkq_read_offset_;
    recvmarkq_read_offset_ = 0;

    if (pending_eof_)
      ec = boost::system::error_code(boost::asio::error::eof);
    return true;
  }

  return false;
}

bool session::write(const boost::asio::const_buffer &data,
                    boost::system::error_code &ec,
                    std::size_t &bytes_transferred)
{
  size_t buffer_length = boost::asio::buffer_size(data);
  bytes_transferred = 0;
  ec = boost::system::error_code();

  if (buffer_length == 0) {
    return true;
  } else if (pending_eof_) {
    ec = boost::system::error_code(boost::asio::error::eof);
    return true;
  } else if (buffer_length > pending_maximum_ - pending_used_) {
    return false;
  }

  if (pending_.empty())
    pending_.resize(pending_maximum_);

  const unsigned char *buffer = boost::asio::buffer_cast<const unsigned char*>(data);

  if (pending_next_ + buffer_length > pending_maximum_) {
      // Two writes; one at the end and one at the beginning
      int avail = pending_maximum_ - pending_next_;

      std::memcpy(&pending_[0] + pending_next_, buffer, avail);
      std::memcpy(&pending_[0], buffer + avail, buffer_length - avail);

      pending_next_ = buffer_length - avail;
  } else {
      // Just one write at the end
      std::memcpy(&pending_[0] + pending_next_, buffer, buffer_length);

      pending_next_ += buffer_length;
  }

  pending_used_ += buffer_length;
  bytes_transferred = buffer_length;

  // Handle send queue processing immediately as we might have something for a new block
  if (running_)
    handle_process_send_queue(boost::system::error_code());

  return true;
}

int session::handle_sendq_head(struct curvecpr_messager *messager,
                               struct curvecpr_block **block_stored)
{
  session *self = static_cast<session*>(messager->cf.priv);

  if (self->sendq_head_exists_) {
    *block_stored = &self->sendq_head_;
    return 0;
  }

  if (self->pending_used_ || self->pending_eof_) {
    curvecpr_bytes_zero(&self->sendq_head_, sizeof(struct curvecpr_block));

    if (!self->pending_.empty()) {
      int requested = std::min(self->pending_used_, self->messager_.my_maximum_send_bytes);

      self->sendq_head_.data_len = requested;
      if (self->pending_current_ + requested > self->pending_maximum_) {
        // Two reads, one from the end and one from the beginning
        int avail = self->pending_maximum_ - self->pending_current_;

        std::memcpy(self->sendq_head_.data, &self->pending_[0] + self->pending_current_, avail);
        std::memcpy(self->sendq_head_.data + avail, &self->pending_[0], requested - avail);

        self->pending_current_ = requested - avail;
      } else {
        // Just one read at the end
        std::memcpy(self->sendq_head_.data, &self->pending_[0] + self->pending_current_, requested);

        self->pending_current_ += requested;
      }

      self->pending_used_ -= requested;
      self->pending_ready_write_.cancel();
    }

    if (self->pending_used_ == 0 && self->pending_eof_)
      self->sendq_head_.eof = CURVECPR_BLOCK_EOF_SUCCESS;
    else
      self->sendq_head_.eof = CURVECPR_BLOCK_STREAM;

    self->sendq_head_exists_ = true;
  }

  return -1;
}

int session::handle_sendq_move_to_sendmarkq(struct curvecpr_messager *messager,
                                            const struct curvecpr_block *block,
                                            struct curvecpr_block **block_stored)
{
  session *self = static_cast<session*>(messager->cf.priv);

  if (!self->sendq_head_exists_ || block != &self->sendq_head_) {
    auto it = self->sendmarkq_.find(const_cast<curvecpr_block*>(block));
    if (it != self->sendmarkq_.end()) {
      // Re-sort block to new position as clock has likely been updated
      self->sendmarkq_.erase(it);
      self->sendmarkq_.insert(const_cast<curvecpr_block*>(block));
    }
    return -1;
  }

  if (handle_sendmarkq_is_full(messager))
    return -1;

  curvecpr_block *new_block = new curvecpr_block();
  *new_block = *block;
  self->sendmarkq_.insert(new_block);

  // We have just removed the head
  self->sendq_head_exists_ = false;

  if (block_stored)
    *block_stored = new_block;

  return 0;
}

unsigned char session::handle_sendq_is_empty(struct curvecpr_messager *messager)
{
  session *self = static_cast<session*>(messager->cf.priv);

  return !self->sendq_head_exists_ && // We don't have a block actually waiting to be written
         self->pending_used_ == 0 &&  // We don't have any bytes that we could turn into a block to be written
         (
           !self->pending_eof_ ||     // The EOF flag is not set
           self->messager_.my_eof     // Even if our EOF flag is set, the messager must not have sent
                                      // the EOF message
         );
}

int session::handle_sendmarkq_head(struct curvecpr_messager *messager,
                                   struct curvecpr_block **block_stored)
{
  session *self = static_cast<session*>(messager->cf.priv);

  if (self->sendmarkq_.empty())
    return -1;

  *block_stored = *self->sendmarkq_.begin();
  return 0;
}

int session::handle_sendmarkq_get(struct curvecpr_messager *messager,
                                  crypto_uint32 acknowledging_id,
                                  struct curvecpr_block **block_stored)
{
  session *self = static_cast<session*>(messager->cf.priv);

  for (curvecpr_block *b : self->sendmarkq_) {
    if (b->id == acknowledging_id) {
      *block_stored = b;
      return 0;
    }
  }

  return -1;
}

int session::handle_sendmarkq_remove_range(struct curvecpr_messager *messager,
                                           unsigned long long start,
                                           unsigned long long end)
{
  session *self = static_cast<session*>(messager->cf.priv);

  for (auto it = self->sendmarkq_.begin(); it != self->sendmarkq_.end();) {
    auto jt = it;
    ++it;

    if ((*jt)->offset >= start && (*jt)->offset + (*jt)->data_len <= end) {
      delete *jt;
      self->sendmarkq_.erase(jt);
    }
  }

  return 0;
}

unsigned char session::handle_sendmarkq_is_full(struct curvecpr_messager *messager)
{
  session *self = static_cast<session*>(messager->cf.priv);

  return self->sendmarkq_.size() >= 0 && self->sendmarkq_.size() >= self->sendmarkq_maximum_;
}

int session::handle_recvmarkq_put(struct curvecpr_messager *messager,
                                  const struct curvecpr_block *block,
                                  struct curvecpr_block **block_stored)
{
  session *self = static_cast<session*>(messager->cf.priv);

  // Check if receive queue is full
  if (self->recvmarkq_.size() >= 0 && self->recvmarkq_.size() >= self->recvmarkq_maximum_)
    return -1;

  curvecpr_block_status *new_block = new curvecpr_block_status();
  new_block->status = RECVMARKQ_ELEMENT_NONE;
  new_block->block = *block;

  self->recvmarkq_.insert(new_block);
  self->pending_ready_read_.cancel();

  if (block_stored)
    *block_stored = &new_block->block;

  return 0;
}

int session::handle_recvmarkq_get_nth_unacknowledged(struct curvecpr_messager *messager,
                                                     unsigned int n,
                                                     struct curvecpr_block **block_stored)
{
  session *self = static_cast<session*>(messager->cf.priv);

  size_t i = 0;
  for (curvecpr_block_status *b : self->recvmarkq_) {
    // Find the nth block that does not have ACKed flag set
    if (!(b->status & RECVMARKQ_ELEMENT_ACKNOWLEDGED)) {
      if (i == n) {
        *block_stored = &b->block;
        return 0;
      } else {
        i++;
      }
    }
  }

  return -1;
}

unsigned char session::handle_recvmarkq_is_empty(struct curvecpr_messager *messager)
{
  session *self = static_cast<session*>(messager->cf.priv);

  return self->recvmarkq_.empty();
}

int session::handle_recvmarkq_remove_range(struct curvecpr_messager *messager,
                                           unsigned long long start,
                                           unsigned long long end)
{
  session *self = static_cast<session*>(messager->cf.priv);

  for (auto it = self->recvmarkq_.begin(); it != self->recvmarkq_.end();) {
    auto jt = it;
    ++it;

    if ((*jt)->block.offset >= start && (*jt)->block.offset + (*jt)->block.data_len <= end) {
      (*jt)->status |= RECVMARKQ_ELEMENT_ACKNOWLEDGED;

      if ((*jt)->status == RECVMARKQ_ELEMENT_DONE) {
        delete *jt;
        self->recvmarkq_.erase(jt);
      }
    }
  }

  return 0;
}

int session::handle_send(struct curvecpr_messager *messager,
                         const unsigned char *buf,
                         size_t num)
{
  session *self = static_cast<session*>(messager->cf.priv);
  self->lower_send_handler_(buf, num);
  return 0;
}

}

}

#endif
