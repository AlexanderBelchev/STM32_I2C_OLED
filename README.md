# STM32_Counter

### Compiling, Flashing and running

The repository has a ``Makefile`` that does all the compiling, linking(I think) and building.  
Make sure that the project is setup correctly to the MCU that is being used by checking the "How to setup" section of this file.  

Run the command 

``make all``  

After success, the next command to run is

``st-util``

This will flash the project and make the chip listen for a debugger on port 4242 by default.  
In a different terminal go to the repository directory again, and run this command

``arm-none-eabi-gdb main.elf``

This will start the debugger.  
Connect the debugger to the chip using

``target extended-remote :4242``

and then load the program using

``load``

and finally run it with

``continue``


## The project

A push button is used to increment a number, then the number is displayed on a 7 segment display

This project was created not because displaying a number on a 7 segment display is hard,  
but because it's a testing ground for "Bare Metal" programming of the STM32.  

This would've been much harder without ***[this](https://vivonomicon.com/2018/04/02/bare-metal-stm32-programming-part-1-hello-arm/)*** Vivionomicon tutorial series.

I faced multiple challenges before I could setup it up successfully to a point where I could  
execute C code and control the GPIO pins. This happened because it's practically a brand new  
concept to me, something I've never done before.

## Components

The components used in project are
+ 1 x STM32F401CCU6 (Mine is a Blackpill copy)
+ 1 x Push button
+ 1 x 0.1uF capacitor
+ 1 x 1K Ohm resistor
+ 1 x 5011AS 7 Segment display
+ 8 x 220 Ohm resistor

### Connection schematic

![Project schematic!](schematics/schematic_image.png "Schematic")

## How to setup

### Linked script

There is a file in this repository named "stm32f401.ld". This is the *linked script*.  
Depending on the chip that is being used (STM32F401, STM32F031, etc.) certain values  
will differ with the ones I am using.
    
    /* Label for the program's entry point */
    ENTRY(reset_handler)

    /* Define the end of RAM and limit of stack memory */
    /* 64KB SRAM (65,536bytes = 0x10000)*/
    /* 256KB FLASH */
    /* RAM starts at address 0x20000000 */
    _estack = 0x20010000;

    /* Minimum size for stack and dynamic memory. */
    /* An error will be generated if there is
     * less than this much RAM lefover */
    /* (1KB) */
    _Min_Leftover_RAM = 0x400;

    MEMORY
    {
      FLASH( rx )         : ORIGIN = 0x08000000, LENGTH = 256K
      RAM ( rxw )         : ORIGIN = 0x20000000, LENGTH = 64K
    }

The biggest difference will be in the variable ``_estack`` which represents the end of the stack in memory.  
This variable is set to the ``RAM origin address + SRAM``. Meaning that if we have 64KB SRAM (65,536bytes), 
to access the last byte we have to address ``0x10000``. So if RAM origin is ``0x20000000`` and SRAM size  
is ``0x10000``, ``_estack`` will have the value ``0x20010000``.  
*This is very easy to mess up, and if it happens, it is very likely that the C code will not execute successfully*  
``FLASH`` and ``RAM`` should also have the correct ``LENGTH`` values.  

### Core.S
This file setups the ``reset_handler``, which loads the program into memory and sets it up to be run.  
I have seen that this functionality has been implemented in the *Vector Table* files that ST provides,  
but this is much more readable because you don't have to work inside the 3k lines vector table.  
The only problem is that you have to remove the ``reset_handler`` implementation that is in the Vector Table.  
``.cpu`` and ``.fpu`` have to be set according to the MCU.  

### Vector table
The vector table is used to setup all the peripherals that the STM32 uses. It's a lot of lines to write by hand,  
and honestly I don't see how one could set up a complete vector table using only the reference manual,  
and thankfully there is no need to do that, because for each STM32 MCU, there is a firmware package that  
can be downloaded from the ST website which contains a complete vector table ready to use, located in:  

``Drivers/CMSIS/Device/ST/STM32[...]/Source/Templates/gcc/startup_stm32[...].s``  
# STM32_I2C_OLED
