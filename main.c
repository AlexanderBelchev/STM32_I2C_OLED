#include "main.h"

//#include <string.h>

#define OLED_ADDR 0x78
//#define OLED_ADDR 0x3C  // Possibly the correct address

typedef struct 
{
    uint8_t cmd;
    uint8_t data[4];
    uint8_t bytes;
} i2c_cmd_t;

/*
 * These commands could be sent in one transfer, but
 * since it's for setup, speed does not matter. This
 * is a bit easier to read
 */
i2c_cmd_t commands[] =
{
    {0x00, {0xAE}, 1},
    {0x00, {0xD5, 0x90}, 2},
    {0x00, {0xA8, 0x3F}, 2},
    {0x00, {0xD3, 0x00}, 2},
    {0x00, {0x40}, 1},
    {0x00, {0xA1}, 1},
    {0x00, {0xC8}, 1},
    {0x00, {0xDA, 0x12}, 2},
    {0x00, {0x81, 0xB0}, 2},
    {0x00, {0xD9, 0x22}, 2},
    {0x00, {0xDB, 0x30}, 2},
    {0x00, {0xA4}, 1},
    {0x00, {0xA6}, 1},
    {0x00, {0x8D, 0x14}, 2},
    {0x00, {0x20, 0x00}, 2}, // Memory Addressing Mode - Horizontal
    {0x00, {0xAF}, 1},
    {0, {0}, 0xff},
};

uint8_t last_pixels[8][128];
uint8_t pixels[8][128];

void oled_write(uint8_t cmd, uint8_t *data, uint8_t len)
{
    uint16_t reg;

    // ------------ START OF MESSAGE ------------
    // Generate start condition
    I2C1->CR1 |= I2C_CR1_START |
        I2C_CR1_ACK;

    while(!(I2C1->SR1 & I2C_SR1_SB)) ; // Wait for SB to clear

    // -------------- ADRESS FRAME --------------
    // Set address, first bit 0 - write
    I2C1->DR = OLED_ADDR;

    while(!(I2C1->SR1 & I2C_SR1_ADDR)); // Wait for ADDR to clear

    reg = I2C1->SR1 | I2C1->SR2; // Clear both status registers

    while(!(I2C1->SR1 & I2C_SR1_TXE)); // Wait for DR to be empty

    // Command byte
    I2C1->DR = cmd;

    while(!(I2C1->SR1 & I2C_SR1_BTF));

    for(int i = 0; i < len; i++)
    {
        while(!(I2C1->SR1 & I2C_SR1_TXE));

        // Data to write
        I2C1->DR = data[i]; // Display ON

        while(!(I2C1->SR1 & I2C_SR1_BTF));
    }
    // Stop transmit
    I2C1->CR1 |= I2C_CR1_STOP;
}

void background_clear(uint8_t state)
{
    for(uint8_t page = 0; page < 8; page++)
    {
        // Find all non "state" pixels and work on them only
        // to save time
        int last_column = 0;
        int first_column = 128;

        for(int x = 0; x < 128; x++)
        {
            if(pixels[page][x] != state)
            {
                if(x < first_column) first_column = x;
                if(x > last_column) last_column = x;

                pixels[page][x] = state;
                //last_pixels[page][x] = state;
            }
        }

        if(first_column > last_column) continue; // Skipp this page, as it needs no chagne

/*        oled_write(0x00, (uint8_t[]){0x22, page, page, 0x21, first_column, last_column}, 6);

        uint8_t bytes = (last_column - first_column) + 1;
        oled_write(0x40, &pixels[page][first_column], bytes);*/
    }
}

void paint_pixel(uint8_t x, uint8_t y, uint8_t state)
{
    uint8_t page = y / 8;
    uint8_t bit = y % 8;

    if(state == 0xFF)
    {
        pixels[page][x] |= 0x1 << bit;
    }
    else
    {
        pixels[page][x] &= ~(0x1<<bit);
    }
}

// Render the pixels buffer
void render()
{
    for(int page = 0; page < 8; page++)
    {
        uint8_t start_column = 128;
        uint8_t end_column = 0;
        // Find the first and last difference in the pixels
        for(int x = 0; x < 128; x++)
        {
            if(pixels[page][x] != last_pixels[page][x])
            {
               if(x < start_column) start_column = x; 
               if(x > end_column) end_column = x;
            }
        }

        if(start_column > end_column) continue; // No change on this PAGE
        
        // Get the difference in start and end. That's the number
        // of bytes we need to send for all the changes
        uint8_t bytes = (end_column - start_column) + 1;
        
        // (Set Page command) {start_page} {end_page} (Set column command) {start_column} {end_column}
        oled_write(0x00, (uint8_t []){ 0x22, page, page, 0x21, start_column, end_column}, 6);

        oled_write(0x40, &pixels[page][start_column], bytes);    
    }

    for(int page = 0; page < 8; page++)
    {
        for(int x = 0; x < 128; x++)
        {
            last_pixels[page][x] = pixels[page][x];
        }
    }
}

/* Main program */
int main(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; // Enable GPIOB

    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN; // Enable I2C

    // Initialize the GPIOB pins
    // Set I2C pins to output

    GPIOB->MODER &= ~((0x3 << (I2C_SCL*2)) |
            (0x3 << (I2C_SDA*2)));

    // Alternative function mode (Needed for I2C to work)
    GPIOB->MODER |=  ((0x2 << (I2C_SCL*2)) |
            (0x2 << (I2C_SDA*2)));   

    GPIOB->PUPDR &= ~((0x3 << (I2C_SCL*2)) |
            (0x3 << (I2C_SDA*2)));

    GPIOB->PUPDR |=  ((0x1 << (I2C_SCL*2)) |
            (0x1 << (I2C_SDA*2)));

    // High speed, setting all bits to 1, so no need to reset 
    GPIOB->OSPEEDR |=  ((0x3 << (I2C_SCL*2)) |
            (0x3 << (I2C_SDA*2)));

    // Open Drain Output
    GPIOB->OTYPER &= ~(0x3 << I2C_SCL) |
        (0x3 << I2C_SDA);
    GPIOB->OTYPER |=  (0x1 << I2C_SCL) |
        (0x1 << I2C_SDA);

    // Set alternative function low register AF4
    // Pin number should be multiplied by 4, according to a tutorial
    GPIOB->AFR[0] |= (0x4 << (I2C_SCL*4)) | 
        (0x4 << (I2C_SDA*4));

    // Reset the I2C
    I2C1->CR1 |= I2C_CR1_SWRST;

    // Normal operation
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    // I2C setup
    // Set Frequency
    I2C1->CR2 |= 0x10; // 16MHz
    I2C1->CR2 |= 0x2A; // 40MHz - FM mode

    // Enable Event Interrupt
    //I2C1->CR2 |= I2C_CR2_ITEVTEN;

    // I2C CCR  setup, only possible when (PE=0)
    //---- Default F/S is Sm
    //---- Default duty cycle
    //I2C1->CCR = 0x50; // 62.5ns(16Mhz) * 80d (0x50) = 5000ns 

    // FM duty cycle will be 0
    I2C1->CCR = I2C_CCR_FS; // Activate FM mode
    I2C1->CCR = 0x35; // 23.8ns(42MHz) * 50ns(0x35) = 1250ns -- FM mode


    // I2C_TRISE setup -- Although the datasheet says that it should be 17 in this case.
    // But I am following a tutorial so idk. This is probably fine.
    //I2C1->TRISE = 0x10; // 1000ns/62.5ns = 16 
    I2C1->TRISE = 0x0E; // 300ns/23.8ns = 13+1  -- FM mode


    // Enable peripheral
    I2C1->CR1 |= I2C_CR1_PE;

    // Init commands
    uint8_t cmd = 0;
    while(commands[cmd].bytes != 0xff)
    {
        oled_write(commands[cmd].cmd, commands[cmd].data, commands[cmd].bytes);
        ++cmd;
    }
    uint8_t x = 0, y = 0;
    background_clear(0xff);
    render();

    while(1)
    {
        // Set background to black
        background_clear(0x00);

        // Draw dick butt
        for(int i = 0; i < (DICKBUTT_HEIGHT * DICKBUTT_WIDTH); i++)
        {
            uint8_t local_x = i % DICKBUTT_WIDTH;
            uint8_t local_y = i / DICKBUTT_WIDTH;

            paint_pixel(local_x + x, local_y + y, dickbutt[i]);
        }
        render();

        // Some delay
        for(int i = 0; i < 200000; i++)
        {
            asm("NOP");
        }
        
        x += 10;
        if(x > 128-54) x = 0; // When the image touches the right edge, reset
    }
}

