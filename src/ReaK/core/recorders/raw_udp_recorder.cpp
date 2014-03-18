
/*
 *    Copyright 2014 Sven Mikael Persson
 *
 *    THIS SOFTWARE IS DISTRIBUTED UNDER THE TERMS OF THE GNU GENERAL PUBLIC LICENSE v3 (GPLv3).
 *
 *    This file is part of ReaK.
 *
 *    ReaK is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    ReaK is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with ReaK (as LICENSE in the root folder).  
 *    If not, see <http://www.gnu.org/licenses/>.
 */

#include "raw_udp_recorder.hpp"

#include <boost/asio.hpp>

#include <iostream>

namespace ReaK {

namespace recorder {


class raw_udp_server_impl {
  public:
    boost::asio::io_service io_service;
    boost::asio::ip::udp::endpoint endpoint;
    boost::asio::ip::udp::socket socket;
    boost::asio::basic_streambuf<> row_buf;
    
    raw_udp_server_impl(const std::string& ip4_address, std::size_t port_num) : 
      io_service(), 
      endpoint(boost::asio::ip::address_v4::from_string(ip4_address), port_num), 
      socket(io_service) {
      socket.open(boost::asio::ip::udp::v4());
      socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
    };
    
};

class raw_udp_client_impl {
  public:
    boost::asio::io_service io_service;
    boost::asio::ip::udp::endpoint endpoint;
    boost::asio::ip::udp::socket socket;
    boost::asio::basic_streambuf<> row_buf;
    
    raw_udp_client_impl(const std::string& ip4_address, std::size_t port_num) : 
      io_service(), 
      endpoint(boost::asio::ip::address_v4::from_string(ip4_address), port_num), 
      socket(io_service) { 
      socket.open(boost::asio::ip::udp::v4());
      socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
      socket.bind(endpoint);
    };
    
};



raw_udp_recorder::raw_udp_recorder() : data_recorder(), pimpl() { };

raw_udp_recorder::raw_udp_recorder(const std::string& aFileName) {
  setFileName(aFileName);
};

raw_udp_recorder::~raw_udp_recorder() { };

void raw_udp_recorder::writeRow() {
  ReaKaux::unique_lock< ReaKaux::mutex > lock_here(access_mutex);
  if((pimpl) && (pimpl->socket.is_open()) && (rowCount > 0) && (colCount > 0)) {
    std::ostream s_tmp(&(pimpl->row_buf));
    {
      for(std::size_t i = 0; i < colCount; ++i) {
        double tmp(values_rm.front());
        s_tmp.write(reinterpret_cast<char*>(&tmp),sizeof(double));
        values_rm.pop();
      };
      --rowCount;
    };
    std::size_t len = pimpl->socket.send_to(pimpl->row_buf.data(), pimpl->endpoint);
    pimpl->row_buf.consume(len);
  };
};

void raw_udp_recorder::writeNames() { };

void raw_udp_recorder::setFileName(const std::string& aFileName) {
  ReaKaux::unique_lock< ReaKaux::mutex > lock_here(access_mutex);
  std::size_t i = aFileName.find(':');
  std::string ip4addr = aFileName.substr(0, i);
  std::size_t portnum = 17000;
  if(++i < aFileName.size()) {
    std::stringstream ss( aFileName.substr(i, aFileName.size() - i) );
    ss >> portnum;
  };
  pimpl = shared_ptr<raw_udp_server_impl>(new raw_udp_server_impl(ip4addr, portnum));
};





raw_udp_extractor::raw_udp_extractor() : data_extractor(), pimpl() { };

raw_udp_extractor::~raw_udp_extractor() {};


void raw_udp_extractor::addName(const std::string& s) { 
  ReaKaux::unique_lock< ReaKaux::mutex > lock_here(access_mutex);
  names.push_back(s); 
  ++colCount;
};


bool raw_udp_extractor::readRow() {
  ReaKaux::unique_lock< ReaKaux::mutex > lock_here(access_mutex);
  shared_ptr<raw_udp_client_impl> pimpl_tmp = pimpl;
  if((pimpl_tmp) && (pimpl_tmp->socket.is_open()) && (colCount > 0)) {
    try {
      boost::asio::streambuf::mutable_buffers_type bufs = pimpl_tmp->row_buf.prepare(colCount * sizeof(double));
      std::size_t len = pimpl_tmp->socket.receive_from(bufs, pimpl_tmp->endpoint);
      pimpl_tmp->row_buf.commit(len);
      if(len < colCount * sizeof(double))
        return false;
    } catch(...) {
      return false;
    };
    std::istream s_tmp(&(pimpl_tmp->row_buf));
    for(std::size_t i = 0; (i < colCount) && (s_tmp); ++i) {
      double tmp = 0;
      s_tmp.read(reinterpret_cast<char*>(&tmp),sizeof(double));
      values_rm.push(tmp);
    };
  };
  return true;
};

bool raw_udp_extractor::readNames() {
  return true;
};

void raw_udp_extractor::setFileName(const std::string& aFileName) {
  ReaKaux::unique_lock< ReaKaux::mutex > lock_here(access_mutex);
  std::size_t i = aFileName.find(':');
  std::string ip4addr = aFileName.substr(0, i);
  std::size_t portnum = 17000;
  if(++i < aFileName.size()) {
    std::stringstream ss( aFileName.substr(i, aFileName.size() - i) );
    ss >> portnum;
  };
  pimpl = shared_ptr<raw_udp_client_impl>(new raw_udp_client_impl(ip4addr, portnum));
};



};


};



