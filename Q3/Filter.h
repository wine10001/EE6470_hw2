#ifndef FILTER_H_
#define FILTER_H_
#include <systemc>
using namespace sc_core;

#include <tlm>
#include <tlm_utils/simple_target_socket.h>

#include "filter_def.h"

class Filter : public sc_module {
public:
  tlm_utils::simple_target_socket<Filter> t_skt;

  sc_fifo<unsigned char> r;
  sc_fifo<unsigned char> g;
  sc_fifo<unsigned char> b;
  sc_fifo<unsigned char> flag;
  sc_fifo<unsigned char> result_r;
  sc_fifo<unsigned char> result_g;
  sc_fifo<unsigned char> result_b;

  SC_HAS_PROCESS(Filter);
  Filter(sc_module_name n);
  ~Filter();

private:
  void do_filter();
  //int val[MASK_N];

  unsigned int base_offset;
  void blocking_transport(tlm::tlm_generic_payload &payload,
                          sc_core::sc_time &delay);
};
#endif
