//
// spdlog Copyright(c) 2015-2018 Gabi Melman, see https://github.com/gabime/spdlog
// this is a simple lazy variant of https://github.com/gabime/spdlog/blob/v1.x/include/spdlog/sinks/basic_file_sink.h
//
// Derived from: https://gist.github.com/gexclaude/4f867c4b288c4f1c1426a2ce654ee3e6
#pragma once

#ifndef SPDLOG_H
#include "spdlog/spdlog.h"
#endif

#include "spdlog/details/file_helper.h"
#include "spdlog/details/null_mutex.h"
#include "spdlog/sinks/base_sink.h"

#include <mutex>
#include <string>

#include <spdlog/sinks/sink.h>

namespace spdlog {
namespace sinks {
/*
 * Trivial lazy initialized file sink with single file as target
 */
template <typename Mutex> class basic_lazy_file_sink final : public base_sink<Mutex> {
public:
  explicit basic_lazy_file_sink(const filename_t &filename, bool truncate = false) {
    filename_ = filename;
    truncate_ = truncate;
  }

protected:
  void sink_it_(const details::log_msg &msg) override {
    if (file_helper_.filename().empty()) {
      // not yet initialized, do it now
      try {
        file_helper_.open(filename_, truncate_);
      } catch (const spdlog_ex &) {
        // file open failed, try fallback log file once
        if (filename_ != "spdlog_fallback.txt") {
          filename_ = "spdlog_fallback.txt";
          file_helper_.open(filename_, truncate_);
          // trade off of a lazy initialized file logger - if last attempt fails exception is thrown and swallowed by
          // spdlog
        }
      }
    }

    if (!file_helper_.filename().empty()) {
      fmt::basic_memory_buffer<char, 250> formatted;
      this->formatter_->format(msg, formatted);
      file_helper_.write(formatted);
    }
  }

  void flush_() override { file_helper_.flush(); }

private:
  filename_t filename_;
  bool truncate_;
  details::file_helper file_helper_;
};

using basic_lazy_file_sink_mt = basic_lazy_file_sink<std::mutex>;
using basic_lazy_file_sink_st = basic_lazy_file_sink<details::null_mutex>;

} // namespace sinks

//
// factory functions
//
template <typename Factory = default_factory>
inline std::shared_ptr<logger> basic_lazy_logger_mt(const std::string &logger_name, const filename_t &filename,
                                                    bool truncate = false) {
  return Factory::template create<sinks::basic_lazy_file_sink_mt>(logger_name, filename, truncate);
}

template <typename Factory = default_factory>
inline std::shared_ptr<logger> basic_lazy_logger_st(const std::string &logger_name, const filename_t &filename,
                                                    bool truncate = false) {
  return Factory::template create<sinks::basic_lazy_file_sink_st>(logger_name, filename, truncate);
}

} // namespace spdlog
