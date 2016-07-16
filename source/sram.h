/* SRAM properties */
#define EXT_SRAM_HIGHEST_ADDRESS_BIT  (16)
#define EXT_SRAM_LOWEST_ADDRESS_BIT   (0)
#define EXT_SRAM_DATA_WIDTH           (16)
#define EXT_SRAM_SIZE                 ( ( 1 << (EXT_SRAM_HIGHEST_ADDRESS_BIT - EXT_SRAM_LOWEST_ADDRESS_BIT + 2) ) - 1 )

/* SRAM Address pins */
#define EXT_SRAM_A0   PA12
#define EXT_SRAM_A1   PA13
#define EXT_SRAM_A2   PA14
#define EXT_SRAM_A3   PB9
#define EXT_SRAM_A4   PB10
#define EXT_SRAM_A5   PC6
#define EXT_SRAM_A6   PC7
#define EXT_SRAM_A7   PE0
#define EXT_SRAM_A8   PE1
#define EXT_SRAM_A9   PE2
#define EXT_SRAM_A10  PE3
#define EXT_SRAM_A11  PE4
#define EXT_SRAM_A12  PE5
#define EXT_SRAM_A13  PE6
#define EXT_SRAM_A14  PE7
#define EXT_SRAM_A15  PC8
#define EXT_SRAM_A16  PB0

/* SRAM Data pins */
#define EXT_SRAM_D0   PE8
#define EXT_SRAM_D1   PE9
#define EXT_SRAM_D2   PE10
#define EXT_SRAM_D3   PE11
#define EXT_SRAM_D4   PE12
#define EXT_SRAM_D5   PE13
#define EXT_SRAM_D6   PE14
#define EXT_SRAM_D7   PE15
#define EXT_SRAM_D8   PA15
#define EXT_SRAM_D9   PA0
#define EXT_SRAM_D10  PA1
#define EXT_SRAM_D11  PA2
#define EXT_SRAM_D12  PA3
#define EXT_SRAM_D13  PA4
#define EXT_SRAM_D14  PA5
#define EXT_SRAM_D15  PA6

/* SRAM control pins */
/* (SRAM configured to bank 0 of EBI) */
#define EXT_SRAM_nCS  PD9
#define EXT_SRAM_nOE  PF5
#define EXT_SRAM_nWE  PF4

#define EXT_SRAM_EBI_BANK   (0x0)

/* Pin setup array */
static const PinName sramSetupArray[(EXT_SRAM_HIGHEST_ADDRESS_BIT - EXT_SRAM_LOWEST_ADDRESS_BIT) + 1 + EXT_SRAM_DATA_WIDTH + 3] = {
    EXT_SRAM_A0,
    EXT_SRAM_A1,
    EXT_SRAM_A2,
    EXT_SRAM_A3,
    EXT_SRAM_A4,
    EXT_SRAM_A5,
    EXT_SRAM_A6,
    EXT_SRAM_A7,
    EXT_SRAM_A8,
    EXT_SRAM_A9,
    EXT_SRAM_A10,
    EXT_SRAM_A11,
    EXT_SRAM_A12,
    EXT_SRAM_A13,
    EXT_SRAM_A14,
    EXT_SRAM_A15,
    EXT_SRAM_A16,
    EXT_SRAM_D0,
    EXT_SRAM_D1,
    EXT_SRAM_D2,
    EXT_SRAM_D3,
    EXT_SRAM_D4,
    EXT_SRAM_D5,
    EXT_SRAM_D6,
    EXT_SRAM_D7,
    EXT_SRAM_D8,
    EXT_SRAM_D9,
    EXT_SRAM_D10,
    EXT_SRAM_D11,
    EXT_SRAM_D12,
    EXT_SRAM_D13,
    EXT_SRAM_D14,
    EXT_SRAM_D15,
    EXT_SRAM_nCS,
    EXT_SRAM_nOE,
    EXT_SRAM_nWE
 };
