#include <curvecp/curvecp.hpp>
#include <boost/asio/write.hpp>
#include <botan/auto_rng.h>

class example {
public:
  example(boost::asio::io_service &service)
    : service_(service),
      stream_(service),
      buffer_space_(1024),
      sent_data_(0)
  {
    stream_.set_local_extension(std::string("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16));
    stream_.set_local_public_key(std::string("\xa3\xe7\xb1\x22\xe6\x86\x77\x7c\x39\xc3\xf8\x76\x3d\x4d\x4\xf\x39\x7\x24\x37\xa3\xf5\x7c\x5d\xfc\x56\x59\xc0\x95\xb7\xc1\x3c", 32));
    stream_.set_local_private_key(std::string("\xd3\x51\x1b\x58\x9c\x33\x8d\xd2\x9e\x50\xe7\x14\xec\xb7\x79\x5d\x23\x51\x33\xe7\x27\x0\x40\xa\x1d\xad\x10\xd2\x4e\xac\x8e\xab", 32));
    stream_.set_remote_extension(std::string("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16));
    stream_.set_remote_public_key(std::string("\x3f\x56\xfd\x60\x4f\x31\x57\x5d\x1f\xa8\xd2\x4\x2e\x8a\xd7\xe1\x1e\x8a\x51\x64\xf0\x79\xb7\x63\x63\x14\xcd\x52\x9e\x7a\x9a\x19", 32));
    stream_.set_remote_domain_name("test.server");
    stream_.set_nonce_generator(boost::bind(&Botan::AutoSeeded_RNG::randomize, &rng_, _1, _2));
  }

  void start()
  {
    stream_.connect(boost::asio::ip::udp::endpoint(
      boost::asio::ip::address::from_string("127.0.0.1"),
      10000
    ));

    for (int i = 0; i < 64; i++) {
      buffer_space_[i] = 104;
    }

    boost::asio::async_write(stream_,
      boost::asio::buffer(&buffer_space_[0], 64),
      boost::bind(&example::write_handler, this, _1)
    );
  }

  void close_handler()
  {
    std::cout << "STREAM: Stream closed." << std::endl;
  }

  void read_handler(const boost::system::error_code &ec, std::size_t bytes)
  {
    if (ec) {
      std::cout << "STREAM: Error ocurred while reading!" << std::endl;
      stream_.async_close(boost::bind(&example::close_handler, this));
      return;
    }

    if (++sent_data_ > 1024) {
      std::cout << "STREAM: Closing stream." << std::endl;
      stream_.async_close(boost::bind(&example::close_handler, this));
      return;
    }

    boost::asio::async_write(stream_,
      boost::asio::buffer(&buffer_space_[0], 64),
      boost::bind(&example::write_handler, this, _1)
    );
  }

  void write_handler(const boost::system::error_code &ec)
  {
    if (ec) {
      std::cout << "STREAM: Error ocurred while writing!" << std::endl;
      stream_.async_close(boost::bind(&example::close_handler, this));
      return;
    }

    boost::asio::async_read(stream_,
      boost::asio::buffer(&buffer_space_[0], 64),
      boost::bind(&example::read_handler, this, _1, _2)
    );
  }
private:
  /// Random number generator
  Botan::AutoSeeded_RNG rng_;
  /// ASIO I/O service
  boost::asio::io_service &service_;
  /// CurveCP stream
  curvecp::stream stream_;
  /// Buffer space
  std::vector<char> buffer_space_;
  /// Sent data counter
  size_t sent_data_;
};

int main()
{
  boost::asio::io_service io_service;
  example ex(io_service);
  ex.start();
  io_service.run();
  return 0;
}
