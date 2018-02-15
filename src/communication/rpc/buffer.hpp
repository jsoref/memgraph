#pragma once

#include <vector>

#include "io/network/stream_buffer.hpp"

namespace communication::rpc {

// Initial capacity of the internal buffer.
const size_t kBufferInitialSize = 65536;

/**
 * @brief Buffer
 *
 * Has methods for writing and reading raw data.
 *
 * Allocating, writing and written stores data in the buffer. The stored
 * data can then be read using the pointer returned with the data function.
 * This implementation stores data in a variable sized array (a vector).
 * The internal array can only grow in size.
 */
class Buffer {
 public:
  Buffer();

  /**
   * Allocates a new StreamBuffer from the internal buffer.
   * This function returns a pointer to the first currently free memory
   * location in the internal buffer. Also, it returns the size of the
   * available memory.
   */
  io::network::StreamBuffer Allocate();

  /**
   * This method is used to notify the buffer that the data has been written.
   * To write data to this buffer you should do this:
   * Call Allocate(), then write to the returned data pointer.
   * IMPORTANT: Don't write more data then the returned size, you will cause
   * a memory overflow. Then call Written(size) with the length of data that
   * you have written into the buffer.
   *
   * @param len the size of data that has been written into the buffer
   */
  void Written(size_t len);

  /**
   * This method shifts the available data for len. It is used when you read
   * some data from the buffer and you want to remove it from the buffer.
   *
   * @param len the length of data that has to be removed from the start of
   *            the buffer
   */
  void Shift(size_t len);

  /**
   * This method resizes the internal data buffer.
   * It is used to notify the buffer of the incoming message size.
   * If the requested size is larger than the buffer size then the buffer is
   * resized, if the requested size is smaller than the buffer size then
   * nothing is done.
   *
   * @param len the desired size of the buffer
   */
  void Resize(size_t len);

  /**
   * This method clears the buffer.
   */
  void Clear();

  /**
   * This function returns a pointer to the internal buffer. It is used for
   * reading data from the buffer.
   */
  uint8_t *data();

  /**
   * This function returns the size of available data for reading.
   */
  size_t size();

 private:
  std::vector<uint8_t> data_;
  size_t have_{0};
};
}  // namespace communication::rpc
