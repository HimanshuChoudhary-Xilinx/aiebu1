// SPDX-License-Identifier: MIT
// Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.

#ifndef _AIEBU_COMMON_UID_MD5_H_
#define _AIEBU_COMMON_UID_MD5_H_

#include <vector>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <cstdint>
#include <sstream>
#include <boost/version.hpp>
#include <boost/uuid/detail/md5.hpp>

namespace aiebu {

unsigned constexpr md5_size = 16;

class uid_md5 {
  boost::uuids::detail::md5 hasher;
  std::vector<char> sig = std::vector<char>(md5_size, 0);

public:
  uid_md5() = default;

  void update(const std::vector<uint8_t>& data)
  {
    hasher.process_bytes(data.data(), data.size());
  }

  const std::vector<char>& calculate()
  {
    // Creating local copy of context, so calculate() return same md5sum on every call.
    boost::uuids::detail::md5 hasher_copy = hasher;
    boost::uuids::detail::md5::digest_type digest;

    hasher_copy.get_digest(digest);
    std::memcpy(sig.data(), digest, md5_size);

    constexpr size_t element_size = sizeof(digest[0]);

    // Boost 1.71+ writes MD5 digest bytes in RFC 1321 order (little-endian per 32-bit
    // word). Older Boost used host-endian byte layout in the digest buffer; the
    // reverse below normalized that for stable output. Applying the reverse on 1.71+
    // scrambles the digest (see boost/uuid/detail/md5.hpp, BOOST_UUID_DETAIL_MD5_BYTE_OUT).

    // digest_type is either unsigned char[16] (element_size == 1) or unsigned int[4]
    // (element_size == 4), depending on Boost. Pre-1.71 vs 1.71+ MD5 byte layout differs.

    // BOOST_VERSION < 107100  and element_size ==1     -- reverse each 4-byte word
    // BOOST_VERSION < 107100  and element_size !=1     -- do not reverse
    // BOOST_VERSION >= 107100  and element_size !=1    -- reverse each element_size chunk
    // BOOST_VERSION >= 107100  and element_size ==1    -- do not reverse
#if BOOST_VERSION < 107100 || defined(BOOST_UUID_COMPAT_PRE_1_71_MD5)
    // Pre-1.71 (or compat): element_size == 1 -> reverse each 4-byte MD5 word;
    // element_size != 1 -> leave sig as memcpy'd from digest.
    if constexpr (element_size == 1) {
      for (size_t i = 0; i < md5_size; i += 4)
        std::reverse(sig.data() + i, sig.data() + i + 4);
    }
#else
    // Boost >= 1.71: element_size != 1 -> reverse each element-sized chunk;
    // element_size == 1 -> leave as memcpy'd.
    if constexpr (element_size != 1) {
      // Different boost versions model digest_type differently:
      // 1. typedef unsigned int(digest_type)[4];
      // 2. typedef unsigned char digest_type[16];
      // For case 1. the following code swaps the bytes of each integer in
      // the signature. This solves the little endian issue of integer bytes
      // stored in the reverse order than in which they are printed.
      auto tcurr = sig.begin();
      auto done = sig.end();
      while (tcurr < done) {
        auto tend = tcurr + element_size;
        std::reverse(tcurr, tend);
        tcurr = tend;
      }
    }
#endif

    return sig;
  }

  [[nodiscard]] std::string str() const {
    std::stringstream md5;

    md5 << std::hex << std::setfill('0');
    for (auto ele : sig) {
      auto c = (unsigned char)ele;
      md5 << std::setw(sizeof(ele) * 2) << (unsigned int)c;
    }
    return md5.str();
  }
};

}
#endif //_AIEBU_COMMON_UID_MD5_H_
