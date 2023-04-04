#include <cmath>
#include <iomanip>

#include "Filter.h"

Filter::Filter(sc_module_name n)
    : sc_module(n), t_skt("t_skt"), base_offset(0) {
  SC_THREAD(do_filter);

  t_skt.register_b_transport(this, &Filter::blocking_transport);
}

const int mask[MASK_X][MASK_Y] = {{1, 1, 1}, 
                                  {1, 2, 1}, 
                                  {1, 1, 1}};

void Filter::do_filter() {
  int data_buffer[3][6] = {0}; // Input data buffer

  while (true) {
    
    int box[3][9] = {0};
    int sorted_box[3][9] = {0};
    int result[3] = {0};
    unsigned char sig = flag.read();
    
    if (sig == 1){
      for (int i = 0; i < 8; i ++){
        sig = flag.read();
      }
    } else {
      for (int i = 0; i < 2; i ++){
        sig = flag.read();
      }
    }

    // Check x cooridinate of the pixel is 0 or not
    if (sig){                               // x cooridinate of the pixel is 0
      for (unsigned int v = 0; v < MASK_Y; ++v) {
        for (unsigned int u = 0; u < MASK_X; ++u) {
          unsigned char color[3] = {r.read(),  g.read(), b.read()};       
          for (unsigned int i = 0; i != 3; ++i) {
            box[i][v*3+u] = color[i];             
            sorted_box[i][v*3+u] = color[i]; 
          }
        }
      }
    } else{                                          // x cooridinate of the pixel is not 0
      for (unsigned int v = 0; v < MASK_Y; ++v) {
        unsigned char color[3] = {r.read(),  g.read(), b.read()}; 
        for (unsigned int u = 0; u < MASK_X; ++u) {      
          for (unsigned int i = 0; i != 3; ++i) {
            if (u==2){
              box[i][v*3+u] = color[i];              
              sorted_box[i][v*3+u] = color[i]; 
            } else{
              box[i][v*3+u] = data_buffer[i][v*2+u];              
              sorted_box[i][v*3+u] = data_buffer[i][v*2+u]; 
            }
          }
        }
      }
    }

    // Store pixels in data_buffer
    for (unsigned int i = 0; i != 3; ++i) {
      data_buffer[i][0] = box[i][1];
      data_buffer[i][1] = box[i][2];
      data_buffer[i][2] = box[i][4];
      data_buffer[i][3] = box[i][5];
      data_buffer[i][4] = box[i][7];
      data_buffer[i][5] = box[i][8];
    }


    // Implement median filter
    for (unsigned int i = 0; i < 3; ++i) {
      std::sort(std::begin(sorted_box[i]), std::end(sorted_box[i]));
      box[i][4] = sorted_box[i][4];
    }

    // Implement mean filter
    for (unsigned int v = 0; v < MASK_Y; ++v) {
      for (unsigned int u = 0; u < MASK_X; ++u) {
        for (unsigned int i = 0; i != 3; ++i) {
          result[i] += box[i][v*3+u] * mask[u][v] / 10; 
        }
      }
    }

    // Write result
    //unsigned char result_char[3] = {result[0], result[1], result[2]};
    unsigned char char1 = result[0];
    unsigned char char2 = result[1];
    unsigned char char3 = result[2];
    result_r.write(char1);
    result_g.write(char2);
    result_b.write(char3);
    
    wait(1 * CLOCK_PERIOD, SC_NS);
  }
}

void Filter::blocking_transport(tlm::tlm_generic_payload &payload,
                                     sc_core::sc_time &delay) {
  sc_dt::uint64 addr = payload.get_address();
  addr = addr - base_offset;
  unsigned char *mask_ptr = payload.get_byte_enable_ptr();
  unsigned char *data_ptr = payload.get_data_ptr();
  word buffer;
  switch (payload.get_command()) {
  case tlm::TLM_READ_COMMAND:                   // 做完filter後，Testbench要從Filter讀資料
    switch (addr) {
    case SOBEL_FILTER_RESULT_ADDR:
      buffer.uc[0] = result_r.read();
      buffer.uc[1] = result_g.read();
      buffer.uc[2] = result_b.read();
      buffer.uc[3] = 0;
      break;
    case SOBEL_FILTER_CHECK_ADDR:
      buffer.uint = result_r.num_available();
      break;
    default:
      std::cerr << "Error! SobelFilter::blocking_transport: address 0x"
                << std::setfill('0') << std::setw(8) << std::hex << addr
                << std::dec << " is not valid" << std::endl;
      break;
    }
    data_ptr[0] = buffer.uc[0];
    data_ptr[1] = buffer.uc[1];
    data_ptr[2] = buffer.uc[2];
    data_ptr[3] = buffer.uc[3];
    delay=sc_time(1, SC_NS);
    break;

  case tlm::TLM_WRITE_COMMAND:                 // 還沒做filter，Testbench要寫資料給Filter
    switch (addr) {
    case SOBEL_FILTER_R_ADDR:
      if (mask_ptr[0] == 0xff) {
        r.write(data_ptr[0]);
      }
      if (mask_ptr[1] == 0xff) {
        g.write(data_ptr[1]);
      }
      if (mask_ptr[2] == 0xff) {
        b.write(data_ptr[2]);
      }
      if (mask_ptr[3] == 0xff) {
        flag.write(data_ptr[3]);
      }
      break;
    default:
      std::cerr << "Error! SobelFilter::blocking_transport: address 0x"
                << std::setfill('0') << std::setw(8) << std::hex << addr
                << std::dec << " is not valid" << std::endl;
      break;
    }
    delay=sc_time(1, SC_NS);
    break;

  case tlm::TLM_IGNORE_COMMAND:
    payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
    return;
  default:
    payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
    return;
  }
  payload.set_response_status(tlm::TLM_OK_RESPONSE); // Always OK
}
