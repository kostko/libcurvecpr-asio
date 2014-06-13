#include <boost/asio.hpp>
#include <curvecp/curvecp.hpp>
#include <botan/auto_rng.h>
#include <list>
#include <thread>

class connection {
public:
  connection(size_t id, boost::asio::io_service &service)
    : id(id),
      stream(service),
      buffer_space(1024),
      received_data(0)
  {
  }
public:
  /// Connection identifier
  size_t id;
  /// Stream
  curvecp::stream stream;
  /// Buffer space
  std::vector<char> buffer_space;
  /// Received bytes counter
  std::size_t received_data;
};

class example {
public:
  example(boost::asio::io_service &service)
    : service_(service),
      acceptor_(service),
      next_connection_id_(0)
  {
    acceptor_.set_local_extension(std::string("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16));
    acceptor_.set_local_public_key(std::string("\x3f\x56\xfd\x60\x4f\x31\x57\x5d\x1f\xa8\xd2\x4\x2e\x8a\xd7\xe1\x1e\x8a\x51\x64\xf0\x79\xb7\x63\x63\x14\xcd\x52\x9e\x7a\x9a\x19", 32));
    acceptor_.set_local_private_key(std::string("\x7a\xa4\x43\x11\x13\x5f\xb8\xe9\x1c\x3e\x2\xd3\x88\xa\x36\xce\xd0\xd8\x79\x99\x9b\xc5\xf7\x8e\x49\x90\x97\xe4\xdf\x6b\x6d\xa9", 32));
    acceptor_.set_nonce_generator(boost::bind(&Botan::AutoSeeded_RNG::randomize, &rng_, _1, _2));
  }

  void start()
  {
    acceptor_.bind(boost::asio::ip::udp::endpoint(
      boost::asio::ip::address::from_string("127.0.0.1"),
      10000
    ));

    boost::shared_ptr<connection> peer(boost::make_shared<connection>(next_connection_id_++, service_));
    acceptor_.async_accept(
      peer->stream,
      boost::bind(&example::accept_handler, this, peer, _1)
    );
    acceptor_.listen();
  }

  void accept_handler(boost::shared_ptr<connection> peer,
                      const boost::system::error_code &ec)
  {
    std::cout << "ACCEPTOR: Accept handler called, we have a new stream!" << std::endl;
    boost::asio::async_read(
      peer->stream,
      boost::asio::buffer(&peer->buffer_space[0], 64),
      boost::bind(&example::read_handler, this, peer, _1, _2)
    );

    // Get ready for the next connection
    boost::shared_ptr<connection> next_peer(boost::make_shared<connection>(next_connection_id_++, service_));
    acceptor_.async_accept(
      next_peer->stream,
      boost::bind(&example::accept_handler, this, next_peer, _1)
    );
  }

  void close_handler(boost::shared_ptr<connection> peer)
  {
    std::cout << "STREAM[" << peer->id << "]: Stream closed." << std::endl;
  }

  void read_handler(boost::shared_ptr<connection> peer,
                    const boost::system::error_code &ec,
                    std::size_t bytes)
  {
    if (ec) {
      std::cout << "STREAM[" << peer->id << "]: Error ocurred while reading!" << std::endl;
      peer->stream.async_close(boost::bind(&example::close_handler, this, peer));
      return;
    }

    peer->received_data += bytes;
    if (peer->received_data % 1024 == 0)
      std::cout << "STREAM[" << peer->id << "]: Read " << peer->received_data << " bytes." << std::endl;

    boost::asio::async_write(
      peer->stream,
      boost::asio::buffer(&peer->buffer_space[0], bytes),
      boost::bind(&example::write_handler, this, peer, _1, _2)
    );
  }

  void write_handler(boost::shared_ptr<connection> peer,
                     const boost::system::error_code &ec,
                     std::size_t bytes)
  {
    if (ec) {
      std::cout << "STREAM[" << peer->id << "]: Error ocurred while writing!" << std::endl;
      peer->stream.async_close(boost::bind(&example::close_handler, this, peer));
      return;
    }

    boost::asio::async_read(
      peer->stream,
      boost::asio::buffer(&peer->buffer_space[0], 64),
      boost::bind(&example::read_handler, this, peer, _1, _2)
    );
  }
private:
  /// Random number generator
  Botan::AutoSeeded_RNG rng_;
  /// ASIO I/O service
  boost::asio::io_service &service_;
  /// CurveCP acceptor
  curvecp::acceptor acceptor_;
  /// Connection counter
  size_t next_connection_id_;
};

int main()
{
  boost::asio::io_service io_service;
  example ex(io_service);
  ex.start();

  std::list<std::shared_ptr<std::thread>> threads;
  for (int i = 0; i < 8; i++) {
    threads.push_back(std::make_shared<std::thread>([&io_service]() { io_service.run(); }));
  }

  for (auto thread : threads)
    thread->join();
  return 0;
}
