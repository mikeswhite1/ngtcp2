/*
 * ngtcp2
 *
 * Copyright (c) 2020 ngtcp2 contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "tls_server_context_wolfssl.h"

#include <iostream>
#include <fstream>
#include <limits>

#include <ngtcp2/ngtcp2_crypto_wolfssl.h>

#include "server_base.h"
#include "template.h"

extern Config config;

TLSServerContext::TLSServerContext() : ssl_ctx_{nullptr} {}

TLSServerContext::~TLSServerContext() {
  if (ssl_ctx_) {
    wolfSSL_CTX_free(ssl_ctx_);
  }
}

WOLFSSL_CTX *TLSServerContext::get_native_handle() const { return ssl_ctx_; }

namespace {
int alpn_select_proto_h3_cb(WOLFSSL *ssl, const unsigned char **out,
                            unsigned char *outlen, const unsigned char *in,
                            unsigned int inlen, void *arg) {
  auto conn_ref =
      static_cast<ngtcp2_crypto_conn_ref *>(wolfSSL_get_app_data(ssl));
  auto h = static_cast<HandlerBase *>(conn_ref->user_data);
  const uint8_t *alpn;
  size_t alpnlen;
  // This should be the negotiated version, but we have not set the
  // negotiated version when this callback is called.
  auto version = ngtcp2_conn_get_client_chosen_version(h->conn());

  switch (version) {
  case QUIC_VER_DRAFT29:
    alpn = reinterpret_cast<const uint8_t *>(H3_ALPN_DRAFT29);
    alpnlen = str_size(H3_ALPN_DRAFT29);
    break;
  case QUIC_VER_DRAFT30:
    alpn = reinterpret_cast<const uint8_t *>(H3_ALPN_DRAFT30);
    alpnlen = str_size(H3_ALPN_DRAFT30);
    break;
  case QUIC_VER_DRAFT31:
    alpn = reinterpret_cast<const uint8_t *>(H3_ALPN_DRAFT31);
    alpnlen = str_size(H3_ALPN_DRAFT31);
    break;
  case QUIC_VER_DRAFT32:
    alpn = reinterpret_cast<const uint8_t *>(H3_ALPN_DRAFT32);
    alpnlen = str_size(H3_ALPN_DRAFT32);
    break;
  case NGTCP2_PROTO_VER_V1:
  case NGTCP2_PROTO_VER_V2_DRAFT:
    alpn = reinterpret_cast<const uint8_t *>(H3_ALPN_V1);
    alpnlen = str_size(H3_ALPN_V1);
    break;
  default:
    if (!config.quiet) {
      std::cerr << "Unexpected quic protocol version: " << std::hex << "0x"
                << version << std::dec << std::endl;
    }
    return SSL_TLSEXT_ERR_ALERT_FATAL;
  }

  for (auto p = in, end = in + inlen; p + alpnlen <= end; p += *p + 1) {
    if (std::equal(alpn, alpn + alpnlen, p)) {
      *out = p + 1;
      *outlen = *p;
      return SSL_TLSEXT_ERR_OK;
    }
  }

  if (!config.quiet) {
    std::cerr << "Client did not present ALPN " << &alpn[1] << std::endl;
  }

  return SSL_TLSEXT_ERR_ALERT_FATAL;
}
} // namespace

namespace {
int alpn_select_proto_hq_cb(WOLFSSL *ssl, const unsigned char **out,
                            unsigned char *outlen, const unsigned char *in,
                            unsigned int inlen, void *arg) {
  auto conn_ref =
      static_cast<ngtcp2_crypto_conn_ref *>(wolfSSL_get_app_data(ssl));
  auto h = static_cast<HandlerBase *>(conn_ref->user_data);
  const uint8_t *alpn;
  size_t alpnlen;
  // This should be the negotiated version, but we have not set the
  // negotiated version when this callback is called.
  auto version = ngtcp2_conn_get_client_chosen_version(h->conn());

  switch (version) {
  case QUIC_VER_DRAFT29:
    alpn = reinterpret_cast<const uint8_t *>(HQ_ALPN_DRAFT29);
    alpnlen = str_size(HQ_ALPN_DRAFT29);
    break;
  case QUIC_VER_DRAFT30:
    alpn = reinterpret_cast<const uint8_t *>(HQ_ALPN_DRAFT30);
    alpnlen = str_size(HQ_ALPN_DRAFT30);
    break;
  case QUIC_VER_DRAFT31:
    alpn = reinterpret_cast<const uint8_t *>(HQ_ALPN_DRAFT31);
    alpnlen = str_size(HQ_ALPN_DRAFT31);
    break;
  case QUIC_VER_DRAFT32:
    alpn = reinterpret_cast<const uint8_t *>(HQ_ALPN_DRAFT32);
    alpnlen = str_size(HQ_ALPN_DRAFT32);
    break;
  case NGTCP2_PROTO_VER_V1:
  case NGTCP2_PROTO_VER_V2_DRAFT:
    alpn = reinterpret_cast<const uint8_t *>(HQ_ALPN_V1);
    alpnlen = str_size(HQ_ALPN_V1);
    break;
  default:
    if (!config.quiet) {
      std::cerr << "Unexpected quic protocol version: " << std::hex << "0x"
                << version << std::dec << std::endl;
    }
    return SSL_TLSEXT_ERR_ALERT_FATAL;
  }

  for (auto p = in, end = in + inlen; p + alpnlen <= end; p += *p + 1) {
    if (std::equal(alpn, alpn + alpnlen, p)) {
      *out = p + 1;
      *outlen = *p;
      return SSL_TLSEXT_ERR_OK;
    }
  }

  if (!config.quiet) {
    std::cerr << "Client did not present ALPN " << &alpn[1] << std::endl;
  }

  return SSL_TLSEXT_ERR_ALERT_FATAL;
}
} // namespace

namespace {
int alpn_select_proto_perf_cb(WOLFSSL *ssl, const unsigned char **out,
                              unsigned char *outlen, const unsigned char *in,
                              unsigned int inlen, void *arg) {
  constexpr static uint8_t alpn[] = "\x4perf";
  size_t alpnlen = str_size(alpn);

  for (auto p = in, end = in + inlen; p + alpnlen <= end; p += *p + 1) {
    if (std::equal(alpn, alpn + alpnlen, p)) {
      *out = p + 1;
      *outlen = *p;
      return SSL_TLSEXT_ERR_OK;
    }
  }

  if (!config.quiet) {
    std::cerr << "Client did not present ALPN " << &alpn[1] << std::endl;
  }

  return SSL_TLSEXT_ERR_ALERT_FATAL;
}
} // namespace

namespace {
int verify_cb(int preverify_ok, X509_STORE_CTX *ctx) {
  // We don't verify the client certificate.  Just request it for the
  // testing purpose.
  return 1;
}
} // namespace

int TLSServerContext::init(const char *private_key_file, const char *cert_file,
                           AppProtocol app_proto) {
  constexpr static unsigned char sid_ctx[] = "ngtcp2 server";

#if defined(DEBUG_WOLFSSL)
  if (!config.quiet) {
    /*wolfSSL_Debugging_ON();*/
  }
#endif

  ssl_ctx_ = wolfSSL_CTX_new(wolfTLSv1_3_server_method());
  if (!ssl_ctx_) {
    std::cerr << "wolfSSL_CTX_new: "
              << wolfSSL_ERR_error_string(wolfSSL_ERR_get_error(), nullptr)
              << std::endl;
    return -1;
  }

  if (ngtcp2_crypto_wolfssl_configure_server_context(ssl_ctx_) != 0) {
    std::cerr << "ngtcp2_crypto_wolfssl_configure_server_context failed"
              << std::endl;
    return -1;
  }

#ifdef WOLFSSL_EARLY_DATA
  wolfSSL_CTX_set_max_early_data(ssl_ctx_, UINT32_MAX);
#endif

  constexpr auto ssl_opts =
      (WOLFSSL_OP_ALL & ~WOLFSSL_OP_DONT_INSERT_EMPTY_FRAGMENTS) |
      WOLFSSL_OP_SINGLE_ECDH_USE | WOLFSSL_OP_CIPHER_SERVER_PREFERENCE;

  wolfSSL_CTX_set_options(ssl_ctx_, ssl_opts);

  if (wolfSSL_CTX_set_cipher_list(ssl_ctx_, config.ciphers) != 1) {
    std::cerr << "wolfSSL_CTX_set_cipher_list: "
              << ERR_error_string(ERR_get_error(), nullptr) << std::endl;
    return -1;
  }

  if (wolfSSL_CTX_set1_curves_list(ssl_ctx_,
                                   const_cast<char *>(config.groups)) != 1) {
    std::cerr << "wolfSSL_CTX_set1_curves_list(" << config.groups << ") failed"
              << std::endl;
    return -1;
  }

  wolfSSL_CTX_set_mode(ssl_ctx_, SSL_MODE_RELEASE_BUFFERS);

  switch (app_proto) {
  case AppProtocol::H3:
    wolfSSL_CTX_set_alpn_select_cb(ssl_ctx_, alpn_select_proto_h3_cb, nullptr);
    break;
  case AppProtocol::HQ:
    wolfSSL_CTX_set_alpn_select_cb(ssl_ctx_, alpn_select_proto_hq_cb, nullptr);
    break;
  case AppProtocol::Perf:
    wolfSSL_CTX_set_alpn_select_cb(ssl_ctx_, alpn_select_proto_perf_cb,
                                   nullptr);
    break;
  }

  wolfSSL_CTX_set_default_verify_paths(ssl_ctx_);

  if (wolfSSL_CTX_use_PrivateKey_file(ssl_ctx_, private_key_file,
                                      SSL_FILETYPE_PEM) != 1) {
    std::cerr << "wolfSSL_CTX_use_PrivateKey_file: "
              << wolfSSL_ERR_error_string(wolfSSL_ERR_get_error(), nullptr)
              << std::endl;
    return -1;
  }

  if (wolfSSL_CTX_use_certificate_chain_file(ssl_ctx_, cert_file) != 1) {
    std::cerr << "wolfSSL_CTX_use_certificate_chain_file: "
              << wolfSSL_ERR_error_string(wolfSSL_ERR_get_error(), nullptr)
              << std::endl;
    return -1;
  }

  if (wolfSSL_CTX_check_private_key(ssl_ctx_) != 1) {
    std::cerr << "wolfSSL_CTX_check_private_key: "
              << wolfSSL_ERR_error_string(wolfSSL_ERR_get_error(), nullptr)
              << std::endl;
    return -1;
  }

  wolfSSL_CTX_set_session_id_context(ssl_ctx_, sid_ctx, sizeof(sid_ctx) - 1);

  if (config.verify_client) {
    wolfSSL_CTX_set_verify(ssl_ctx_,
                           WOLFSSL_VERIFY_PEER | WOLFSSL_VERIFY_CLIENT_ONCE |
                               WOLFSSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                           verify_cb);
  }

  return 0;
}

extern std::ofstream keylog_file;

#ifdef HAVE_SECRET_CALLBACK
namespace {
void keylog_callback(const WOLFSSL *ssl, const char *line) {
  keylog_file.write(line, strlen(line));
  keylog_file.put('\n');
  keylog_file.flush();
}
} // namespace
#endif

void TLSServerContext::enable_keylog() {
#ifdef HAVE_SECRET_CALLBACK
  wolfSSL_CTX_set_keylog_callback(ssl_ctx_, keylog_callback);
#endif
}
