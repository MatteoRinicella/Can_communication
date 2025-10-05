#include <string.h>

extern "C"{
    #include "mcp_2515_ll.h"
}
#include "stm32f4xx_hal.h"

#include <can/frame.hpp>
#include <can/guards.hpp>
#include <can/mcp2515.hpp>
#include <cstdarg> // per le funzioni con un numero variabile di argomenti
#include <cstdio>

extern UART_HandleTypeDef huart2;
extern SPI_HandleTypeDef hspi1;

using namespace can;

static inline void log(const char *s){
    HAL_UART_Transmit(&huart2, (uint8_t *)s, (uint16_t)strlen(s), HAL_MAX_DELAY);
}

// funzione per il debug dei registri
static void dump_regs(){
    uint8_t cs = mcp_read(MCP_CANSTAT);
    uint8_t ci= mcp_read(MCP_CANCTRL);
    uint8_t ifr = mcp_read(MCP_CANINTF);
    uint8_t t0 = mcp_read(MCP_TXB0CTRL);
    uint8_t r0 = mcp_read(MCP_RXB0CTRL);
    uint8_t tec = mcp_read(MCP_TEC);
    uint8_t rec = mcp_read(MCP_REC);
    uint8_t eflg = mcp_read(MCP_EFLG);
    uint8_t osm = ci & 0x08;
    uint8_t rx0if = ifr & 0x01;
    uint8_t rx1if = ifr & 0x02 ;

    char b[180];

    int n = snprintf(b, sizeof(b), "REGISTER CONFIGURATION -> CANSTAT=0X%02X, CANCTRL=0X%02X, CANINTF=0X%02X, TXB0CTRL=0X%02X, RXB0CTRL=0X%02X, RX0IF=%u, RX1IF=%u, TEC = 0X%02X, REC = 0X%02X, EFLG = 0X%02X, OSM = %u\r\n", cs, ci, ifr, t0, r0, rx0if ? 1: 0, rx1if ? 1: 0, tec, rec, eflg, osm ? 1: 0);
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

//funzione test 
extern "C" void run_milestone_B(){
    log("\n");
    log("----------------\r\n");
    log("CONNECTION START\r\n");
    log("----------------\r\n");
    log("\n");

    MCP2515 dev; // dev -> device
    log("-------------------\r\n");
    log("START RESET SESSION\r\n") ;
    log("-------------------\r\n");
    dev.reset(); // chiamo mcp_reset (file mcp2515.hpp), faccio un software reset
    log("\n");
    dump_regs();
    
    log("--------------------\r\n");
    log("START CONFIG SESSION\r\n") ;
    log("--------------------\r\n");
    (void)dev.setMode(Mode::Config);
    log("\n");
    dump_regs();
    log("--------------\r\n");
    log("SET BIT TIMING\r\n") ;
    log("--------------\r\n");
    (void)dev.setBitTiming500k_8MHz(); // programmo i registri CNF per 500kbit/s con oscillatore a 8MHz
    log("\n");
    dump_regs();
    
    log("-----------------------\r\n");
    log("NORMAL OPERATING MODE\r\n") ;
    log("-----------------------\r\n");
    log("\n");

    { //inizio scope
        ModeGuard loop {Mode::Normal};
       // dev.set_filter(0X7E0);
    }
}