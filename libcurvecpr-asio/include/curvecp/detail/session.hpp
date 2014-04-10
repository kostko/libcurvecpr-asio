/*
 * Copyright (C) 2014 Jernej Kos (jernej@kos.mx)
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef CURVECP_ASIO_DETAIL_SESSION_HPP
#define CURVECP_ASIO_DETAIL_SESSION_HPP

#include <curvecp/detail/curvecpr.h>

#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>

#include <cstdint>
#include <set>
#include <vector>

namespace curvecp {

namespace detail {

/**
 * An internal CurveCP session implementation.
 */
class session {
public:
  /**
   * Type of CurveCP session.
   */
  enum class type {
    // Client session
    client,
    // Server session
    server
  };

  /**
   * Type of event an operation wants.
   */
  enum class want {
    // Returned by operations to signal that an operation has been completed
    nothing,
    // Returned by operations to signal that additional reads need to be performed
    read,
    // Returned by operations to signal that additional writes need to be performed
    write
  };

  /**
   * Constructs an internal CurveCP session implementation.
   *
   * @param session_type Session type
   * @param ready_read_handler Handler for ready read events
   * @param ready_write_handler Handler for ready write events
   * @param lower_send_handler Handler for sending messages
   */
  template <typename ReadReadyHandler, typename WriteReadyHandler, typename LowerSendHandler>
  session(type session_type,
          ReadReadyHandler ready_read_handler,
          WriteReadyHandler ready_write_handler,
          LowerSendHandler lower_send_handler);

  /**
   * Configures the maximum size of pending write buffer.
   *
   * @param value Buffer size
   */
  void set_pending_maximum(std::size_t value) { pending_maximum_ = value; }

  /**
   * Configures the maximum number of unacknowledged sent blocks.
   *
   * @param value Maximum number of unacknowledged sent blocks
   */
  void set_sendmarkq_maximum(std::size_t value) { sendmarkq_maximum_ = value; }

  /**
   * Configures the maximum number of unacknowledged received blocks.
   *
   * @param value Maximum number of unacknowledged received blocks
   */
  void set_recvmarkq_maximum(std::size_t value) { recvmarkq_maximum_ = value; }

  /**
   * Returns true if the session is finished.
   */
  bool is_finished() const;

  /**
   * Marks the session as finished.
   */
  void finish();

  /**
   * Closes this session.
   */
  void close();

  /**
   * Handles receive event from underlying CurveCP client/server.
   */
  int lower_receive(const unsigned char *buf, size_t num);

  /**
   * Processes the send queue.
   */
  int process_send_queue();

  /**
   * Returns the time duration when send queue should be processed.
   */
  boost::posix_time::time_duration get_next_send_timeout();

  /**
   * Performs a read on this session.
   *
   * @param data Destination buffer to read into
   * @param ec Resulting error code
   * @param bytes_transferred Resulting number of bytes transferred
   * @return True when read has been completed, false when it must be retried
   */
  bool read(const boost::asio::mutable_buffer &data,
            boost::system::error_code &ec,
            std::size_t &bytes_transferred);

  /**
   * Performs a write on this session.
   *
   * @param data Source buffer to read from
   * @param ec Resulting error code
   * @param bytes_transferred Resulting number of bytes transferred
   * @return True when write has been completed, false when it must be retried
   */
  bool write(const boost::asio::const_buffer &data,
             boost::system::error_code &ec,
             std::size_t &bytes_transferred);
protected:
  /**
   * Internal handler for libcurvecpr.
   */
  static int handle_sendq_head(struct curvecpr_messager *messager,
                               struct curvecpr_block **block_stored);

  /**
   * Internal handler for libcurvecpr.
   */
  static int handle_sendq_move_to_sendmarkq(struct curvecpr_messager *messager,
                                            const struct curvecpr_block *block,
                                            struct curvecpr_block **block_stored);

  /**
   * Internal handler for libcurvecpr.
   */
  static unsigned char handle_sendq_is_empty(struct curvecpr_messager *messager);

  /**
   * Internal handler for libcurvecpr.
   */
  static int handle_sendmarkq_head(struct curvecpr_messager *messager,
                                   struct curvecpr_block **block_stored);

  /**
   * Internal handler for libcurvecpr.
   */
  static int handle_sendmarkq_get(struct curvecpr_messager *messager,
                                  crypto_uint32 acknowledging_id,
                                  struct curvecpr_block **block_stored);

  /**
   * Internal handler for libcurvecpr.
   */
  static int handle_sendmarkq_remove_range(struct curvecpr_messager *messager,
                                           unsigned long long start,
                                           unsigned long long end);

  /**
   * Internal handler for libcurvecpr.
   */
  static unsigned char handle_sendmarkq_is_full(struct curvecpr_messager *messager);

  /**
   * Internal handler for libcurvecpr.
   */
  static int handle_recvmarkq_put(struct curvecpr_messager *messager,
                                  const struct curvecpr_block *block,
                                  struct curvecpr_block **block_stored);

  /**
   * Internal handler for libcurvecpr.
   */
  static int handle_recvmarkq_get_nth_unacknowledged(struct curvecpr_messager *messager,
                                                     unsigned int n,
                                                     struct curvecpr_block **block_stored);

  /**
   * Internal handler for libcurvecpr.
   */
  static unsigned char handle_recvmarkq_is_empty(struct curvecpr_messager *messager);

  /**
   * Internal handler for libcurvecpr.
   */
  static int handle_recvmarkq_remove_range(struct curvecpr_messager *messager,
                                           unsigned long long start,
                                           unsigned long long end);

  /**
   * Internal handler for libcurvecpr.
   */
  static int handle_send(struct curvecpr_messager *messager,
                         const unsigned char *buf,
                         size_t num);
private:
  /// Internal libcurvecpr messager handle
  curvecpr_messager messager_;
  /// Maximum size of pending write buffer
  std::size_t pending_maximum_;
  /// Maximum number of unacknowledged sent blocks
  std::size_t sendmarkq_maximum_;
  /// Maximum number of unacknowledged received blocks
  std::size_t recvmarkq_maximum_;
  /// Pending write buffer
  std::vector<unsigned char> pending_;
  /// Pending EOF marker
  bool pending_eof_;
  /// Amount of pending buffer used
  std::uint64_t pending_used_;
  /// Amount of buffer pending for inclusion into block
  std::uint64_t pending_current_;
  /// Amount of buffer pending for inclusion into next block
  std::uint64_t pending_next_;
  /// True when a head block exists for sending
  bool sendq_head_exists_;
  /// Head block for sending
  curvecpr_block sendq_head_;

  /**
   * An extended block that contains a status flag.
   */
  struct curvecpr_block_status {
    /// Block
    curvecpr_block block;
    /// Status flag
    unsigned char status;
  };

  /**
   * Comparison operator that compares block clocks (time when block was
   * last sent).
   */
  struct block_clock_compare {
    bool operator()(curvecpr_block *a, curvecpr_block *b) const
    {
      return a->clock < b->clock;
    }
  };

  /**
   * Comparison operator that compares block ranges.
   */
  struct block_range_compare {
    bool operator()(curvecpr_block_status *a, curvecpr_block_status *b) const
    {
      if (a->block.offset == b->block.offset) {
        return a->block.data_len < b->block.data_len;
      } else {
        return a->block.offset < b->block.offset;
      }
    }
  };

  /// Sorted unacknowledged sent blocks
  std::multiset<curvecpr_block*, block_clock_compare> sendmarkq_;
  /// Sorted unacknowledged received blocks (pending distribution)
  std::multiset<curvecpr_block_status*, block_range_compare> recvmarkq_;
  /// Offset of data that has been distributed to upper layers via reads
  std::uint64_t recvmarkq_distributed_;
  /// Offset into the current read buffer
  std::size_t recvmarkq_read_offset_;
  /// Ready read handler
  std::function<void()> ready_read_handler_;
  /// Ready write handler
  std::function<void()> ready_write_handler_;
  /// Lower send handler
  std::function<bool(const unsigned char*, std::size_t)> lower_send_handler_;
};

}

}

#include <curvecp/detail/impl/session.ipp>

#endif
