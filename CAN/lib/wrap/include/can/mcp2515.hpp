#pragma once

#include <cstdint>
#include <can/frame.hpp>
#include <can/guards.hpp>

extern "C"{
    #include "mcp_2515_ll.h"
}

namespace can{

    class MCP2515{
        public:

            void reset() noexcept{mcp_reset();}
            bool setMode(Mode m) noexcept {return mcp_set_mode(static_cast<uint8_t>(m));} // Mode m perchè serve il cast per parlare con la LL C

            bool setBitTiming500k_8MHz() noexcept{return mcp_set_bit_timing_500k_8MHz();}

            void rx0AcceptAll() noexcept {return mcp_rx0_accept_all();}

            bool sendStd(const Frame& f) noexcept;
            bool pollStd(Frame& out) noexcept; //Frame& out -> è un riferimento non costante che la funzione copia solo in caso di successo
            bool set_filter(uint16_t flt) noexcept{return mcp_rx0_set_filter_std(flt);} 
    };
}