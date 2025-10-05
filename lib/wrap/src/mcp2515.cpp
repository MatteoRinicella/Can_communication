#include <can/mcp2515.hpp>

namespace can
{
    bool MCP2515::sendStd(const Frame& f) noexcept{
        if(!f.is_valid()) return false;
        if(!mcp_txb0_load_std(f.id, f.data, f.dlc)) return false; // In pratica chiamo LL C per scrivere i registri del TX buffer 0(SIDH/SIDL, DLC, D=..D7). l'if fallisce se TXB0 è occupato (bit TXREQ alto) o se c'è un errore -> la funzione non si blocca, semplicemente mi dice "ritenta più tardi".
        return mcp_rts_txb0(); // "Request to send" su TXB0: dico al chip di trasmettere quanto appena caricato. Ritorna l'esito della richiesta(propgato come bool della funzione)
    } // TX: carico TXB0 e faccio RTS

    bool MCP2515::pollStd(Frame& out) noexcept{
        uint16_t id = 0;
        uint8_t dlc = 0; // qui alloco le variabili locali dove la LL C metterà i campi letti da RXB=0: ID (11 bit), DLC (0...8), e fino a 8 byte di dati. 
        uint8_t buf[8] = {};

        if(!mcp_poll_rx0(&id, buf, &dlc)) // qui chiamo la LL: c'è un frame in RXB0? Se si -> la LL riempie id, buf, dlc ( e tipicamente azzera il flag RX0IF)
            return false;

        out.id = static_cast<uint16_t>(id & 0X07FF); // qui maschero a 11 bit per essere certo che l'id sia standard 
        out.dlc = (dlc <= 8) ? dlc : 8;
        for(uint8_t i = 0; i < out.dlc; i++) out.data[i] = buf[i];
        
        return true;
    } // RX: controllo RXB0, se c'è un frame, la LL regge RXB0 e me lo riporta in id/dlc/buf.
} 
