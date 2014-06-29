#include <curvecp/curvecp.hpp>
#include <boost/asio/write.hpp>
#include <sodium.h>
#include <list>

class example {
public:
  example(boost::asio::io_service &service, int id)
    : service_(service),
      id_(id),
      stream_(service),
      rx_buffer_space_(1024),
      tx_buffer_space_(1024),
      received_data_(0),
      sent_data_(0)
  {
    stream_.set_local_extension(std::string("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16));
    stream_.set_local_public_key(std::string("\xa3\xe7\xb1\x22\xe6\x86\x77\x7c\x39\xc3\xf8\x76\x3d\x4d\x4\xf\x39\x7\x24\x37\xa3\xf5\x7c\x5d\xfc\x56\x59\xc0\x95\xb7\xc1\x3c", 32));
    stream_.set_local_private_key(std::string("\xd3\x51\x1b\x58\x9c\x33\x8d\xd2\x9e\x50\xe7\x14\xec\xb7\x79\x5d\x23\x51\x33\xe7\x27\x0\x40\xa\x1d\xad\x10\xd2\x4e\xac\x8e\xab", 32));
    stream_.set_remote_extension(std::string("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16));
    stream_.set_remote_public_key(std::string("\x3f\x56\xfd\x60\x4f\x31\x57\x5d\x1f\xa8\xd2\x4\x2e\x8a\xd7\xe1\x1e\x8a\x51\x64\xf0\x79\xb7\x63\x63\x14\xcd\x52\x9e\x7a\x9a\x19", 32));
    stream_.set_remote_domain_name("test.server");
    stream_.set_nonce_generator(randombytes);
  }

  void start()
  {
    stream_.async_connect(
      boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 10000),
      boost::bind(&example::connect_handler, this, _1)
    );
  }

  std::ostream &log()
  {
    std::cout << "STREAM[" << id_ << "]: ";
    return std::cout;
  }

  void connect_handler(const boost::system::error_code &ec)
  {
    if (ec) {
      log() << "Connection failed." << std::endl;
      return;
    }

    log() << "Connected." << std::endl;

    for (int i = 0; i < 64; i++) {
      tx_buffer_space_[i] = 104;
    }

    // Start reading data
    boost::asio::async_read(stream_,
      boost::asio::buffer(&rx_buffer_space_[0], 64),
      boost::bind(&example::read_handler, this, _1, _2)
    );

    // Write some data as well
    boost::asio::async_write(stream_,
      boost::asio::buffer(&tx_buffer_space_[0], 64),
      boost::bind(&example::write_handler, this, _1)
    );
  }

  void close_handler()
  {
    log() << "Stream closed." << std::endl;
  }

  void read_handler(const boost::system::error_code &ec, std::size_t bytes)
  {
    if (ec) {
      log() << "Error ocurred while reading!" << std::endl;
      stream_.async_close(boost::bind(&example::close_handler, this));
      return;
    }

    if (bytes != 64)
      log() << "WARNING: Received an invalid number of bytes!" << std::endl;

    for (int i = 0; i < bytes; i++) {
      if (rx_buffer_space_[i] != 104)
        log() << "WARNING: Received corrupted byte in position " << i << "!" << std::endl;
    }

    received_data_ += bytes;
    if (received_data_ % 1024 == 0)
      log() << "Read " << received_data_ << " bytes." << std::endl;

    if (received_data_ > 1024*1024) {
      log() << "Closing stream after reading " << received_data_ << " bytes." << std::endl;
      stream_.async_close(boost::bind(&example::close_handler, this));
      return;
    }

    boost::asio::async_read(stream_,
      boost::asio::buffer(&rx_buffer_space_[0], 64),
      boost::bind(&example::read_handler, this, _1, _2)
    );
  }

  void write_handler(const boost::system::error_code &ec)
  {
    if (ec) {
      log() << "Error ocurred while writing!" << std::endl;
      stream_.async_close(boost::bind(&example::close_handler, this));
      return;
    }

    sent_data_ += 64;
    if (sent_data_ % 1024 == 0)
      log() << "Wrote " << sent_data_ << " bytes." << std::endl;

    boost::asio::async_write(stream_,
      boost::asio::buffer(&tx_buffer_space_[0], 64),
      boost::bind(&example::write_handler, this, _1)
    );
  }
private:
  /// ASIO I/O service
  boost::asio::io_service &service_;
  /// Client identifier
  int id_;
  /// CurveCP stream
  curvecp::stream stream_;
  /// RX buffer space
  std::vector<char> rx_buffer_space_;
  /// TX buffer space
  std::vector<char> tx_buffer_space_;
  /// Received data counter
  std::size_t received_data_;
  /// Sent data counter
  std::size_t sent_data_;
};

int main()
{
  boost::asio::io_service io_service;

  std::list<std::shared_ptr<example>> clients;
  for (int i = 0; i < 10; i++) {
    auto client = std::make_shared<example>(io_service, i);
    clients.push_back(client);
    client->start();
  }

  io_service.run();
  return 0;
}
