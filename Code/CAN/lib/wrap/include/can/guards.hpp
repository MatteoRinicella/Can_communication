#pragma once

#include <cstdint>

extern "C"{
    #include "mcp_2515_ll.h"
}

namespace can{

    enum class Mode : uint8_t{ // enum è un tipo definito dall'utente, che rappresenta un insieme finito di costanti nominate, chiamate enumeratori.
        Normal = MCP_MODE_NORMAL,
        Sleep = MCP_MODE_SLEEP,
        Loopback = MCP_MODE_LOOPBACK,
        ListenOnly = MCP_MODE_LISTEN_ONLY,
        Config = MCP_MODE_CONFIG
    };

    class ModeGuard{
        public: 
            explicit ModeGuard(Mode target) noexcept
            : prev_{current()} // lista di inizializzazione 
            {
                (void)mcp_set_mode(static_cast<uint8_t>(target));
            }

            ~ModeGuard() noexcept{ // ~ indica il distruttore della classe
                if(active_){ // mcp_set_mode chiama la funzione c di basso livello che scrive la modalità nel registro CANCTRL.REQOP(richiesta di cambio modalità)
                    (void)mcp_set_mode(static_cast<uint8_t>(prev_)); //in pratica finché active_ è true, i guard è armato e, in distruzione, deve rispristinare la modalità precedente.
                }    // se invece l'utente ha chiamato release(), active_ diventa false -> non ripristina nulla(mantiene la modalità corrente
            }

            // queste due righe disabilitano la copia e l'assegnazione per copia 
            ModeGuard(const ModeGuard&) = delete;
            ModeGuard& operator = (const ModeGuard&) = delete;

            void release() noexcept {active_ = false;} // disattivo il ripristino del distruttore

            static Mode current() noexcept{
                const uint8_t cs = mcp_read(MCP_CANSTAT);
                return static_cast<Mode>(cs & MCP_CANCTRL_REQOP_MASK); // con cs & MCP_CANCTRL_REQOP_MASK applico la mask 0X7E0, che in canstat sono opmode, invece con static_cast<Mode> converto il valore mascherato (0X00, 0X20, 0X40 ecc..) nel mio enum Mode.
            }

            // Modalità che verrà ripristinata se non faccio release
            Mode previous() const noexcept {return prev_;}

            private:
                bool active_{true};
                Mode prev_{Mode::Config}; // sto inizializzando lo stato nella modalià config
    }
;}