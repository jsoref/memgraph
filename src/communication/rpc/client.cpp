#include "boost/archive/binary_iarchive.hpp"
#include "boost/archive/binary_oarchive.hpp"
#include "boost/serialization/access.hpp"
#include "boost/serialization/base_object.hpp"
#include "boost/serialization/export.hpp"
#include "boost/serialization/unique_ptr.hpp"

#include "communication/rpc/client.hpp"

namespace communication::rpc {

Client::Client(const io::network::Endpoint &endpoint,
               const std::string &service_name)
    : endpoint_(endpoint), service_name_(service_name) {}

std::unique_ptr<Message> Client::Call(std::unique_ptr<Message> request) {
  std::lock_guard<std::mutex> guard(mutex_);

  uint32_t request_id = ++next_message_id_;

  // Check if the connection is broken (if we haven't used the client for a
  // long time the server could have died).
  if (socket_ && socket_->ErrorStatus()) {
    socket_ = std::experimental::nullopt;
  }

  // Connect to the remote server.
  if (!socket_) {
    socket_.emplace();
    buffer_.Clear();
    if (!socket_->Connect(endpoint_)) {
      LOG(ERROR) << "Couldn't connect to remote address: " << endpoint_;
      socket_ = std::experimental::nullopt;
      return nullptr;
    }

    socket_->SetKeepAlive();

    // Send service name size.
    MessageSize service_len = service_name_.size();
    if (!socket_->Write(reinterpret_cast<uint8_t *>(&service_len),
                        sizeof(MessageSize), true)) {
      LOG(ERROR) << "Couldn't send service name size!";
      socket_ = std::experimental::nullopt;
      return nullptr;
    }

    // Send service name.
    if (!socket_->Write(service_name_)) {
      LOG(ERROR) << "Couldn't send service name!";
      socket_ = std::experimental::nullopt;
      return nullptr;
    }
  }

  // Send current request ID.
  if (!socket_->Write(reinterpret_cast<uint8_t *>(&request_id),
                      sizeof(uint32_t), true)) {
    LOG(ERROR) << "Couldn't send request ID!";
    socket_ = std::experimental::nullopt;
    return nullptr;
  }

  // Serialize and send request.
  std::stringstream request_stream(std::ios_base::out | std::ios_base::binary);
  {
    boost::archive::binary_oarchive request_archive(request_stream);
    request_archive << request;
    // Archive destructor ensures everything is written.
  }

  const std::string &request_buffer = request_stream.str();
  CHECK(request_buffer.size() <= std::numeric_limits<MessageSize>::max())
      << fmt::format(
             "Trying to send message of size {}, max message size is {}",
             request_buffer.size(), std::numeric_limits<MessageSize>::max());

  MessageSize request_data_size = request_buffer.size();
  if (!socket_->Write(reinterpret_cast<uint8_t *>(&request_data_size),
                      sizeof(MessageSize), true)) {
    LOG(ERROR) << "Couldn't send request size!";
    socket_ = std::experimental::nullopt;
    return nullptr;
  }

  if (!socket_->Write(request_buffer)) {
    LOG(INFO) << "Couldn't send request data!";
    socket_ = std::experimental::nullopt;
    return nullptr;
  }

  // Receive response.
  while (true) {
    auto buff = buffer_.Allocate();
    auto received = socket_->Read(buff.data, buff.len);
    if (received <= 0) {
      socket_ = std::experimental::nullopt;
      return nullptr;
    }
    buffer_.Written(received);

    if (buffer_.size() < sizeof(uint32_t) + sizeof(MessageSize)) continue;
    uint32_t response_id = *reinterpret_cast<uint32_t *>(buffer_.data());
    MessageSize response_data_size =
        *reinterpret_cast<MessageSize *>(buffer_.data() + sizeof(uint32_t));
    size_t response_size =
        sizeof(uint32_t) + sizeof(MessageSize) + response_data_size;
    buffer_.Resize(response_size);
    if (buffer_.size() < response_size) continue;

    std::unique_ptr<Message> response;
    {
      std::stringstream response_stream(std::ios_base::in |
                                        std::ios_base::binary);
      response_stream.str(std::string(
          reinterpret_cast<char *>(buffer_.data() + sizeof(uint32_t) +
                                   sizeof(MessageSize)),
          response_data_size));
      boost::archive::binary_iarchive response_archive(response_stream);
      response_archive >> response;
    }

    buffer_.Shift(response_size);

    if (response_id != request_id) {
      // This can happen if some stale response arrives after we issued a new
      // request.
      continue;
    }

    return response;
  }
}

void Client::Abort() {
  if (!socket_) return;
  // We need to call Shutdown on the socket to abort any pending read or
  // write operations.
  socket_->Shutdown();
  socket_ = std::experimental::nullopt;
}

}  // namespace communication::rpc
