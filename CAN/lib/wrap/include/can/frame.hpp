#pragma once

#include <cstdint> // mi definisce i tipi interi a larghezza fissa, quindi gli uint8_t/16_t e 
#include <cstddef> //utile per esempio per fare la differenza tra due puntatori 
#include <initializer_list> // per costruire i membri prima ancora di entrare nel corpo del costruttore

extern "C"{
    #include "mcp_2515_ll.h"
}

namespace can{
    
    struct Frame{
        uint16_t id{0};
        uint8_t dlc{0};
        uint8_t data[8]{};

        constexpr bool is_valid() const noexcept{
            return (id <= 0X7FF) && (dlc <= 8);
        }

        void set_data(const uint8_t *src, uint8_t n) noexcept{ // src è il puntatore sorgente da cui la funzione legge i byte del payload 
            if(!src){
                dlc = 0; //se src==nullptr, non c'è sorgente -> nessun payload -> dlc=0 e si esce
                return;
            }
            if(n > 8)
                n = 8; //mai oltre gli 8 byte (CAN classico)
            
            dlc = n;

            for(uint8_t i = 0; i < n; ++i){
                data[i] = src[i];
            }
        }

        // costruzione del frame standard(11 bit) con payload passato a lista 
        static Frame make_std(uint16_t sid, std::initializer_list<uint8_t> bytes) noexcept{ // con static Frame in pratica vado a utilizzare tutti i controlli che ho definito precedentemente (all'interno della struct stessa)
            Frame f; // make_std è static -> non ha this. Quindi per impostare d, dlc e riempire data[8] mi serve un oggetto Frame su cui mettere i valori. Quell'oggetto è f
            f.id = static_cast<uint16_t>(sid&0X7FF); // il cast è ridondante, è una sicurezza in più
            uint8_t n = static_cast<uint8_t>(bytes.size() > 8 ? 8 : bytes.size()); // con l'operatore ternario sto dicendo: se bytes.size() > 8 -> usa 8, altrimenti usa bytes.size(). Esempi: {0x11,0x22,0x33} → bytes.size()==3 → n=3   {0,1,2,3,4,5,6,7} → bytes.size()==8 → n=8   {0..9} (10 elementi) → bytes.size()==10 → n=8 (tronca a 8)
            f.dlc = n;

            uint8_t i = 0;
            for(uint8_t b: bytes){ //scorre tutti gli elementi della lista che ho passato std:: initializer_list<uint8_t> bytes. Quindi a ogni giro, b è una copia del prossimo byte della lista, definisco uint come counter perché int può contare miliardi, invece a me ne servono 8. 
                if(i >= n) break;// stop di sicurezza, esce dal ciclo quando già ho copiato n byte
                    f.data[i++] = b; // copia b dentro il buffer del frame alla posizione i. i++ è post-incremento, usa i per l'indice e poi incrementa i di 1 per il prossimo giro. Quindi riempio data[0], data[1], data[2],...
            }
            return f;
        } 

        // che ho ricevuto esattamente cio' che ho trasmesso
        bool equals(const Frame& other) const noexcept{ // Frame&= alias (riferimento a un Frame già esistente), quindi non crea una copia dell'oggetto, si lavora sul "chiamante". const davanti al tipo = non posso modificarlo dentro la funzione 
            if(id != other.id || dlc != other.dlc)  return false;
            for(uint8_t i = 0; i< dlc; i++)
                if(data[i] != other.data[i])    return false;
            return true;
        }

        template <typename PrintfLike>
        void log_tx(PrintfLike printf_like) const noexcept{
            printf_like("TX: id=0X%03X dlc=%u, data= ", id, dlc); // id stampa in esadecimale a 3 cifre , dlc in decimale
            for(uint8_t i = 0; i < dlc; i++)
                printf_like("%02X%s", data[i], (i+1 < dlc) ? " ": "");
            printf_like("\r\n");
        }

        template <typename PrintfLike>
        void log_rx(PrintfLike printf_like) const noexcept{
            printf_like("RX: id=0X%03X dlc=%u, data= ", id, dlc); //%u -> unsigned int, %03X -> stampa in esadecimale a 3 cifre
            for(uint8_t i = 0; i < dlc; i++)
                printf_like("%02X%s", data[i], (i+1 < dlc)? " ": "");
            printf_like("\r\n");
        }
    };

}
