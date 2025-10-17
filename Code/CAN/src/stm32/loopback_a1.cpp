#include <string.h>

extern "C"{
    #include "mcp_2515_ll.h"
}
#include "stm32f4xx_hal.h"
#include <can/frame.hpp>
#include <can/guards.hpp>
#include <can/mcp2515.hpp>

extern UART_HandleTypeDef huart2;
extern SPI_HandleTypeDef hspi1;

using namespace can;

#include <cstdarg> // per le funzioni con un numero variabile di argomenti
#include <cstdio> 

static inline void log(const char *s){
    HAL_UART_Transmit(&huart2, (uint8_t *)s, (uint16_t)strlen(s), HAL_MAX_DELAY);
}

/*void spi_loopback_selftest(void){
    const uint8_t v[] = {0x00,0xFF,0xA5,0x5A,0x3C,0xC3};
    for (unsigned i=0;i<sizeof(v);++i){
        uint8_t rx = mcp_spi_txrx(v[i]);
        if (rx != v[i]) {
            HAL_UART_Transmit(&huart2,(uint8_t*)"SPI LOOPBACK FAIL\r\n",19,HAL_MAX_DELAY);
            return;
        }
    }
    HAL_UART_Transmit(&huart2,(uint8_t*)"SPI LOOPBACK OK\r\n",17,HAL_MAX_DELAY);
}*/

// funzione per il debug dei registri
static void dump_regs(){
    uint8_t cs = mcp_read(MCP_CANSTAT);
    uint8_t ci= mcp_read(MCP_CANCTRL);
    uint8_t ifr = mcp_read(MCP_CANINTF);
    uint8_t t0 = mcp_read(MCP_TXB0CTRL);
    uint8_t r0 = mcp_read(MCP_RXB0CTRL);
    uint8_t rx0if = ifr & 0x01;
    uint8_t rx1if = ifr & 0x02 ;

    char b[160];

    int n = snprintf(b, sizeof(b), "REGISTER CONFIGURATION -> CANSTAT=0X%02X, CANCTRL=0X%02X, CANINTF=0X%02X, TXB0CTRL=0X%02X, RXB0CTRL=0X%02X, RX0IF=%u, RX1IF=%u\r\n", cs, ci, ifr, t0, r0, rx0if ? 1: 0, rx1if ? 1: 0);
    HAL_UART_Transmit(&huart2, (uint8_t*)b, n, 100);
    log("\n");
}

static int my_printf(const char* fmt, ...) {// con i puntini indico un numero di argomenti variabile, dato che il numero degli argomenti posso variare a seconda del formato
    char buf[128];
    va_list ap; // va_list -> variable argument list 
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if(n < 0) return n; // se n e' negativo, significa che non c'e' una stringa valida e quindi con return esco dalla funzione e rimando un valore al chiamante -> quindi non chiamo la HAL_UART_Trasmit
    if(n > (int)sizeof(buf)) n = sizeof(buf); // quindi se n e' piu' grande della capienza del buffer, si clampa a sizeof(buf) in modo da non chiedere alla UART di inviare piu' dei byte disponibili in memoria
    HAL_UART_Transmit(&huart2, reinterpret_cast<uint8_t*>(buf), (uint16_t)n, 100);
    return n;
} 

//funzione di test
 extern "C" void run_a1_loopback_500k(){ // verra' richiamata dal main
    log("\n");
    log("----------------\r\n");
    log("Connection start\r\n");
    log("----------------\r\n");
    log("\n");

    MCP2515 dev; // dev -> device
    log("-------------------\r\n");
    log("Start reset session\r\n") ;
    log("-------------------\r\n");
    
    dev.reset(); // chiamo mcp_reset (file mcp2515.hpp), faccio un software reset
    log("\n");
    dump_regs();
    log("--------------------\r\n");
    log("Start Config session\r\n") ;
    log("--------------------\r\n");
    (void)dev.setMode(Mode::Config); // il void mi serve per zittire il warning "valore di ritorno bool non usato"
    log("\n");
    dump_regs();
    log("--------------\r\n");
    log("Set bit timing\r\n") ;
    log("--------------\r\n");
    (void)dev.setBitTiming500k_8MHz(); // programmo i registri CNF per 500kbit/s con oscillatore a 8MHz
    log("\n");
    dump_regs();
    dev.rx0AcceptAll(); // imposto maschere/filtri di RXB0 a "accetta tutto" (utile nel bring-up e in loopback) e pulisce RX0IF, cosi' qualunque frame (compreso in loopback) finsice in RXB0 senza essere scartato.
    
    log("-----------------------\r\n");
    log("LOOPBACK OPERATING MODE\r\n") ;
    log("-----------------------\r\n");
    { // inizio scope 
        ModeGuard loop{Mode::Loopback};
        log("\n");
        dump_regs();
        Frame tx = Frame::make_std(0X321,{0X44,0X32,0X66,0x87});
        tx.log_tx(my_printf); // quindi qui passo a log_tx la funzione my_printf

        (void)dev.sendStd(tx); // invio l'opcode -> request to send
        //dump_regs();
        HAL_Delay(1);

        //lettura e log del frame ricevuto
        Frame rx; // alloco un frame vuoto su stack, cioè creo un contenitore (sarà vuoto finchè non lo riempirò) 
        if(dev.pollStd(rx)){ // controllo RXB0, se non c'è nulla torna false, se un frame standard ((11bit)) è presente, lo copia dentro rx(ID,DLC, data) e ritorna true
            log("\n");
            rx.log_rx(my_printf); //  stampo via uart il frame ricevuto
            
        }
        else{
            
            my_printf("RX: timeout/empty\r\n");
            
        }

    } // fine scope 
    
    
}

