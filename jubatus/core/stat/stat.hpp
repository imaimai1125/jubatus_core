// Jubatus: Online machine learning framework for distributed environment
// Copyright (C) 2012 Preferred Networks and Nippon Telegraph and Telephone Corporation.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef JUBATUS_CORE_STAT_STAT_HPP_
#define JUBATUS_CORE_STAT_STAT_HPP_

#include <stdint.h>
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <string>
#include <utility>
#include "jubatus/util/concurrent/rwmutex.h"
#include "jubatus/util/data/unordered_map.h"
#include "jubatus/util/lang/enable_shared_from_this.h"
#include "jubatus/util/lang/shared_ptr.h"
#include "../common/version.hpp"
#include "../common/exception.hpp"
#include "../common/unordered_map.hpp"
#include "../framework/mixable_helper.hpp"

namespace jubatus {
namespace core {
namespace stat {

class stat_error : public common::exception::jubaexception<stat_error> {
 public:
  explicit stat_error(const std::string& msg)
      : msg_(msg) {
  }
  ~stat_error() throw() {
  }

  const char* what() const throw() {
    return msg_.c_str();
  }

 private:
  std::string msg_;
};

class stat : public jubatus::util::lang::enable_shared_from_this<stat> {
 public:
  explicit stat(size_t window_size);
  virtual ~stat();

  virtual void get_diff(std::pair<double, size_t>& ret) const;
  virtual bool put_diff(const std::pair<double, size_t>&);
  virtual void mix(
      const std::pair<double, size_t>& lhs,
      std::pair<double, size_t>& ret) const;

  void push(const std::string& key, double val);

  double sum(const std::string& key) const;
  double stddev(const std::string& key) const;
  double max(const std::string& key) const;
  double min(const std::string& key) const;

  virtual double entropy() const;
  double moment(const std::string& key, int n, double c) const;

  virtual void clear();
  storage::version get_version() const {
    return storage::version();
  }

  virtual void pack(framework::packer& packer) const;
  virtual void unpack(msgpack::object o);
  std::string type() const;

 protected:
  struct stat_val {
    stat_val()
        : n_(0),
          sum_(0),
          sum2_(0),
          max_(0),
          min_(0) {
    }

    void add(double d) {
      n_ += 1;
      sum_ += d;
      sum2_ += d * d;

      if (n_ > 1) {
        max_ = std::max(max_, d);
      } else {
        max_ = d;
      }

      if (n_ > 1) {
        min_ = std::min(min_, d);
      } else {
        min_ = d;
      }
    }

    void rem(double d, const std::string& key, stat& st) {
      n_ -= 1;
      sum_ -= d;
      sum2_ -= d * d;
      if (max_ == d) {
        if (n_ > 0) {
          bool first = true;
          for (size_t i = 0; i < st.window_.size(); ++i) {
            if (st.window_[i].second.first != key) {
              continue;
            }
            double d = st.window_[i].second.second;
            if (first) {
              max_ = d;
              first = false;
            } else {
              max_ = std::max(max_, d);
            }
          }
        } else {
          max_ = 0;
        }
      }
      if (min_ == d) {
        if (n_ > 0) {
          bool first = true;
          for (size_t i = 0; i < st.window_.size(); ++i) {
            if (st.window_[i].second.first != key) {
              continue;
            }
            double d = st.window_[i].second.second;
            if (first) {
              min_ = d;
              first = false;
            } else {
              min_ = std::min(min_, d);
            }
          }
        } else {
          min_ = 0;
        }
      }
    }

    size_t n_;

    double sum_, sum2_;
    double max_;
    double min_;

    MSGPACK_DEFINE(n_, sum_, sum2_, max_, min_);
  };

  std::deque<std::pair<uint64_t, std::pair<std::string, double> > > window_;
  jubatus::util::data::unordered_map<std::string, stat_val> stats_;

 private:
  size_t window_size_;

  double e_;
  double n_;

 public:
  MSGPACK_DEFINE(window_size_, window_, stats_, e_, n_);
};

typedef framework::linear_mixable_helper<stat, std::pair<double, size_t> >
    mixable_stat;

}  // namespace stat
}  // namespace core
}  // namespace jubatus

#endif  // JUBATUS_CORE_STAT_STAT_HPP_
