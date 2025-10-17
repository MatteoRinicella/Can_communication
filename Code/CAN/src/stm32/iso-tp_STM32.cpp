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

static inline void message(const char *c){
    HAL_UART_Transmit(&huart2, (uint8_t*) c, (uint16_t)strlen(c), 1500);
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

extern "C" void run_iso_tp(){
    uint8_t buff[8] = {};
    uint8_t counter_vin = 0;
    uint8_t size = 0;
    uint16_t id{0};
    uint8_t dlc{0};
    uint8_t data[8]{};
    uint8_t buff2[8] = {};
    bool wait = false;
    bool ok = true;
    message("\n");
    message("----------------\r\n");
    message("CONNECTION START\r\n");
    message("----------------\r\n");
    message("\n");

    MCP2515 dev;
    dev.reset();
    (void)dev.setMode(Mode::Config);
    (void)dev.setBitTiming500k_8MHz();

    {   //inizio scope
        
        // 7E0 tester, 7E8 ECU
        dev.set_filter(0X7E8); // ciò significa che deve ascolatre soltanto le risposte dell'ECU con ID pari a 0X7E8
        ModeGuard loop {Mode::Normal};

        log("-----------------------\r\n");
        log("NORMAL MODE READY\r\n") ;
        log("-----------------------\r\n");
        log("\n");

        log("-------------------\r\n");
        log("START COMMUNICATION\r\n") ;
        log("-------------------\r\n");
        log("\n");

        //Frame tx2 = Frame::make_std(0X7E0,{0X02, 0X3E,0X80});
        //Frame tx2 = Frame::make_std(0X7E0,{0X02, 0X3E,0X00});
        //Frame tx2 = Frame::make_std(0X7E0,{0X03, 0X22, 0XF1, 0X90});
        Frame tx2 = Frame::make_std(0X7E0,{0X03, 0X22, 0XF2, 0X90});

        (void)dev.sendStd(tx2);
        tx2.log_tx(my_printf);

        /////// INIZIO MULTIFRAME

        Frame rx;
        if(dev.pollStd(rx)){
            //rx.log_rx(my_printf);
            
            if((rx.data[2] == 0X62) && (rx.data[3] == 0XF1) && (rx.data[4] == 0X90)){
                for(uint8_t i = 0; i < sizeof(buff); i++) buff[i] = rx.data[i];
            }
            else if(rx.data[0] == 0X7F){
                for(uint8_t i = 0; i < sizeof(buff); i++) buff[i] = rx.data[i];
            }
            
            //else if (rx.data[0] == 0X31) wait = true;

            my_printf("RX: id= 0X%03X, dlc= %u, data= ", rx.id, rx.dlc);
            for(uint8_t i = 0; i < sizeof(buff); i++) my_printf("%02X ", buff[i]);
            log("\r\n");      
        }    

            Frame FC1 = Frame::make_std(0X7E0,{0X30, 0X00, 0X00, 0x55, 0x55, 0x55, 0x55, 0x55});
            (void)dev.sendStd(FC1);
            FC1.log_tx(my_printf);
        
            //RICEZIONE CF
            if(FC1.data[0] == 0X31){
                HAL_Delay(3000);
            }
            else if(FC1.data[0] == 0X32){
                HAL_Delay(1000);
                log("\r\n");
                log("OVF ERROR!\r\n");
            }
            
        
            uint32_t t0 = HAL_GetTick();
            while(HAL_GetTick() - t0 < 1500){
            Frame rx1;
                if(dev.pollStd(rx1)){
                    HAL_Delay(100);
                    for(uint8_t i = 0; i < 8; i++) buff2[i] = rx1.data[i];
                        my_printf("RX: id= 0X%03X, dlc= %u, data= ", rx1.id, rx1.dlc);
                    for(uint8_t i = 0; i < sizeof(buff2); i++) my_printf("%02X ", buff2[i]);
                        log("\r\n");
                }
            }
            
            /////// FINE MULTI-FRAME

            ////// INIZIO SINGLE-FRAME

           /* uint32_t t0 = HAL_GetTick();
            const uint32_t T0 = 100;
            while((HAL_GetTick()-t0)<T0){
            uint16_t id; uint8_t dlc; uint8_t data[8]; 
                if(mcp_poll_rx0(&id, data, &dlc)){ // provo a leggere nel buffer rx0
                my_printf("RX: id=0X%03X dlc=%u\r\n",id, dlc);
                    if(id==0X7E8){
                        uint8_t L = 0, uds[8];
                        if(is_sf_and_extract(data, dlc, &L, uds) && L >= 2 && uds[0] == 0X7E && uds[1] == 0X00){
                            my_printf("UDS service ok\r\n");
                        }
                        else
                        my_printf("Error, unrecognized UDS service\r\n");
                    }
                    break;
                }
            }*/
            
             /////// FINE SINGLE-FRAME
        
        log("\n");
        log("-----------------\r\n");
        log("END COMMUNICATION\r\n") ;
        log("-----------------\r\n");
        log("\n");
    }
}