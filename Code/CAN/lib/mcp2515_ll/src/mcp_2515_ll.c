#if !defined(ARDUINO)
#include <string.h>
#include "stm32f4xx_hal.h"
#include "mcp_2515_ll.h"

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart2;   

//SETTING DEL CHIP SELECT
#define MCP_CS_GPIO GPIOB
#define MCP_CS_PIN GPIO_PIN_6

static inline void mcp_cs_low(void) {HAL_GPIO_WritePin(MCP_CS_GPIO, MCP_CS_PIN, GPIO_PIN_RESET);}
static inline void mcp_cs_high(void) {HAL_GPIO_WritePin(MCP_CS_GPIO, MCP_CS_PIN, GPIO_PIN_SET);}

static inline void ll_print(const char* s){
    HAL_UART_Transmit(&huart2, (uint8_t*)s, (uint16_t)strlen(s), HAL_MAX_DELAY);
}

uint8_t mcp_spi_txrx(uint8_t b){ //byte che devo trasmettere su MOSI
    uint8_t rx = 0; // byte locale per ricevere su MISO
    HAL_SPI_TransmitReceive(&hspi1, &b, &rx, 1,HAL_MAX_DELAY);  // puntatore al buffer TX, //puntatore al buffer RX, numero di byte da scambiare 
    return rx; // restituisco  il byte ricevuto su MISO
} // Quindi mando il byte b su MOSI, contemporaneamente leggo un byte da MISO in RX, restituisco il byte ricevuto. Quindi b è il dummy

void mcp_reset(){
    mcp_cs_low();
    mcp_spi_txrx(MCP_RESET);
    mcp_cs_high();
    HAL_Delay(2);
}

uint8_t mcp_read(uint8_t addr){
    mcp_cs_low();
    mcp_spi_txrx(MCP_READ);
    mcp_spi_txrx(addr); //spedisco l'indirizzo del registro da leggere 
    uint8_t v = mcp_spi_txrx(0XFF); //dummy per MOSI 
    mcp_cs_high();

    return v;
}

void mcp_write(uint8_t addr, uint8_t val){ 
    mcp_cs_low();
    mcp_spi_txrx(MCP_WRITE);
    mcp_spi_txrx(addr);
    mcp_spi_txrx(val);
    mcp_cs_high();
}

void mcp_bit_modify(uint8_t addr, uint8_t mask, uint8_t data){
    mcp_cs_low();
    mcp_spi_txrx(MCP_BIT_MOD);
    mcp_spi_txrx(addr);
    mcp_spi_txrx(mask);
    mcp_spi_txrx(data);
    mcp_cs_high();
}

bool mcp_set_mode(uint8_t mode){
    mcp_bit_modify(MCP_CANCTRL, MCP_CANCTRL_REQOP_MASK, mode); //1) registro di controllo del chip, i vit 5...7 di CANCTRL si chiamano REQOP e indicano la modalità operativ richiesta, 2) mask (0XE0) per toccare soltanto i bit 5...7

    for(int i = 0; i < 50; i++){
        uint8_t canstat = mcp_read(MCP_CANSTAT); //verifico che il cambiamento avvenga entro 50ms, leggendo il registro di stato 

        if((canstat & MCP_CANCTRL_REQOP_MASK) == (mode & MCP_CANCTRL_REQOP_MASK))   return true; // confronto i registri di stato e di cambio modalità, se combaciano torna true (50 confronti)
            
        HAL_Delay(1);
    }
    return false;
}

bool mcp_set_bit_timing_500k_8MHz(void){
    if((mcp_read(MCP_CANSTAT) & MCP_CANCTRL_REQOP_MASK) != MCP_MODE_CONFIG)
        return false;
// questi registri si possono scrivere soltanto in configuration mode
    mcp_write(MCP_CNF1, 0X00);
    mcp_write(MCP_CNF2, 0X91);
    mcp_write(MCP_CNF3, 0X01); 

    return true; 
}

void mcp_rx0_accept_all(void){
    mcp_bit_modify(MCP_RXB0CTRL, 0X60, 0X60); // RXB-> registro di controllo del buffer, 11 -> acetta tutto

    mcp_bit_modify(MCP_CANINTF, 0X01, 0X00); 
}

void mcp_id_to_regs_std(uint16_t id, uint8_t *sidh, uint8_t *sidl){
    id &= 0x7FF; // mask per selezionare soltanto i bit da 0 a 10

    *sidh = (uint8_t)(id >> 3);
    *sidl = (uint8_t)((id << 5)& 0XE0); // prima shifto e poi maskero con 0XE0 = 1110 0000b per tenere i bit 5...7
}

bool mcp_txb0_load_std(uint16_t id, const uint8_t *data, uint8_t len){ 
    if(len > 8)
        return false; // se chiedo più di 8 byte la funzione rifiuta 

    if(mcp_read(MCP_TXB0CTRL) & (0X08)) // controllo attraverso TXREQ che TXB0 sia libero
        return false;

    uint8_t sidh, sidl; // creo i contenitori
    mcp_id_to_regs_std(id, &sidh, &sidl); // passo gli indirizzi dei contenitori

    // burst write (riempio in un colpo tutti i resgistri dentro TXB0), l'MCP autoincrementa l'indirizzo interno a ogni byte finchè tengo cs basso
    mcp_cs_low();
    mcp_spi_txrx(MCP_WRITE);
    mcp_spi_txrx(MCP_TXB0SIDH); // da qui in poi l'indirizzo avanza da solo: SIDH, SIDL, EID8, EID0, ...pag 63
    mcp_spi_txrx(sidh); // contiene SID10...SID3
    mcp_spi_txrx(sidl); // contiene SID2...SID0
    mcp_spi_txrx(0X00); // EID8, lo setto a zero 
    mcp_spi_txrx(0X00); // EID0, lo setto a zero
    mcp_spi_txrx(len & 0X0F); // qui maskero il bit DLC0...DLC3 (pag22), questi campi indicano la grandezza il payload del frame 

    for(uint8_t i = 0; i< len; i++) // e poi abbiamo i campi TXB0D0...TXB0D7, ovvero i byte del payload 
        mcp_spi_txrx((data[i])); 
    
    mcp_cs_high();

    return true;
}

bool mcp_rts_txb0(void){
    mcp_cs_low();
    mcp_spi_txrx(MCP_RTS_TXB0); // 0X81
    mcp_cs_high();

    return true;
}

bool mcp_poll_rx0(uint16_t *id, uint8_t *data, uint8_t *len){ // interrogo senza interrupt
     if((mcp_read(MCP_CANINTF) & 0X01) == 0) // verifico se c'è un frame in arrivo
        return false;

    mcp_cs_low();
    mcp_spi_txrx(MCP_READ); // leggo i registri 
    mcp_spi_txrx(MCP_RXB0SIDH);

    uint8_t sidh = mcp_spi_txrx(0XFF); // TX dummy, RX SIDH. Qui mi serve il byte ricevuto, quindi assegno il valore di ritorno a una variabile
    uint8_t sidl = mcp_spi_txrx(0XFF);
    (void)mcp_spi_txrx(0XFF); // EID8. Qui invece non mi serve il valore, sto leggendo EID8 (stessa cosa vale per EID0) soltanto per avanzare all'indirizzo successivo  
    (void)mcp_spi_txrx(0XFF);
    uint8_t dlc = mcp_spi_txrx(0XFF);

    uint8_t n = dlc & 0X0F; // mask per i campi DLC (pag.22)

    if(n > 8)
        n = 8; 
    
    if(dlc & 0X40) // significa che il bit6 = 1 ⇒ RTR ⇒ payload assente
        n = 0;

    for(uint8_t i = 0; i < n; i++)
        data[i] = mcp_spi_txrx(0XFF); // leggo da D0 a D(n-1)
    
    mcp_cs_high();

    if(sidl & 0X08){ // mi serve per bloccare gli ID estesi
        mcp_bit_modify(MCP_CANINTF, 0X01, 0X00); // CANINTF-> registro dove l'MCP mette i flags di evento, 0X01-> mask (selziono RX0IF) 0X00 -> sui bit selezionati scrivo 0. In pratica mi azzera RX0IF e non tocca nessun altro bit di CANINTF
        return false;
    }

    *id = (uint16_t)(((uint16_t)sidh << 3) | (sidl >> 5)) & 0X7FF; // con il primo shift porto i bit SID10...SID3 nelle posizioni 10..3 della variabile, con il secondo prto i bit SID2..0 nelle poszioni 0..3 della variabile, dopo di che poer sicurezza maskero qualsiasi cosa al di fuori degli 11 bit
    *len = n;
    mcp_bit_modify(MCP_CANINTF, 0X01, 0X00);

    return true;
}

bool mcp_rx0_set_filter_std(uint16_t flt){
    uint8_t sidh, sidl;

    if((mcp_read(MCP_CANSTAT) & MCP_CANCTRL_REQOP_MASK) != MCP_MODE_CONFIG) return false; // i filtri e le maschere possono essere applicati soltanto in modalità CONFIG

    mcp_write(MCP_RXM0SIDH, 0XFF); // per confrontare tutti e 8 bit di SIDH (SID10...SID3)
    mcp_write(MCP_RXM0SIDL, 0XE0 | 0X08); // per confrontare i bit di SIDL (SID2...SID0) , in più metto l'indirizzo di EXIDE per assicurarmi di accetare soltanto bit standard 

    
    mcp_id_to_regs_std(flt, &sidh, &sidl); // richiamo la funzione che mi gestisce l'id e setto l'unico id che voglio accettare 
    mcp_write(MCP_RXF0SIDH, sidh); //quindi qui scrivo la parte alta del registro
    mcp_write(MCP_RXF0SIDL, (uint8_t)(sidl & 0XE0)); //qui invece quella bassa 

    mcp_bit_modify(MCP_RXB0CTRL, 0X60,0X00); // cioè qui sto dicendo di settare i bit che corrispondono a RXM0 e RXM1 a 00 -> 0X60 = 0110 0000 -> mask
    return true;
}

// questa funzione mi serve per capire se il frame can (da max 8 byte) è SF e se lo è lo estrae
bool is_sf_and_extract(const uint8_t *can_data, uint8_t dlc, uint8_t *outlen, uint8_t *out_payload){ 
    if(dlc < 1) return false;  

    uint8_t pci = can_data[0]; //controllo il primo byte che mi indica il tipo di frame
    if((pci >> 4) != 0X00) return false; //con lo shift i bit del nibble più alto vengono shiftati nelle posizione b3...b0 
    uint8_t L = pci & 0X0F; // isolo con una mask i bit che mi interessano, ovvero il nibble meno signficativo che mi indica la lunghezza del messaggio 
    if(L== 0 || L > 7) return false; 
    
    if(dlc < (uint8_t)(1+L)) return false; // mi assicuro che nel buffer ci siano abbastanza byte per leggerli

    for(uint8_t i = 0; i < L; i++) out_payload[i] = can_data[i+1];
    for(uint8_t i = L; i < 7; i++) out_payload[i] = 0X55;
    
    *outlen = L;
    return true;
}

bool is_mf_and_extract(const uint8_t *can_data, uint8_t dlc){
    if(dlc < 1) return false;

    uint8_t pci = can_data[1];
    if((pci >> 4) == 0X00) return false;
    uint8_t L = pci & 0X0F;
    if(L <= 7) return false;

    if(dlc < (uint8_t)(1 + L)) return false; 
}

#endif