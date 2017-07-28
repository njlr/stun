#pragma once

#include <common/Util.h>

#include <string.h>
#include <unistd.h>

#include <vector>

namespace networking {

template <int L> struct Packet {
  static const size_t capacity = L;

  size_t size;
  Byte* data;

  Packet() : size(0) { data = new Byte[L]; }

  ~Packet() {
    if (data != nullptr) {
      delete[] data;
    }
  }

  Packet(Packet const& copy) : size(copy.size) {
    data = new Byte[L];
    memcpy(data, copy.data, copy.size);
  }

  Packet(Packet&& move) : size(move.size) {
    data = move.data;
    move.data = nullptr;
  }

  Packet& operator=(Packet other) {
    std::swap(size, other.size);
    std::swap(data, other.data);
  }

  void fill(Byte* buffer, size_t size) {
    this->size = size;
    memcpy(data, buffer, size);
  }

  template <typename T> void pack(T const& obj) {
    fill((Byte*)&obj, sizeof(obj));
  }

  template <typename T> T unpack() {
    T obj;
    memcpy(&obj, data, sizeof(obj));
    return obj;
  }
};
}
