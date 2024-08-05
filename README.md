# raspi4_freertos

This repository includes a FreeRTOS UART-2 shell application which can run on Raspberry Pi 4B.

## 1. Overview

This FreeRTOS porting uses UART2 (PL011) running independently.

## 2. Repository Setup

1. Clone the repository:
    ```bash
    git clone https://github.com/TImada/raspi4_freertos.git
    ```

2. Download the patch file `uart2_shell_support.patch`.

3. Apply the patch:
    ```bash
    git apply uart2_shell_support.patch
    ```

4. Navigate to the UART directory:
    ```bash
    cd Demo/CORTEX_A72_64-bit_Raspberrypi4/uart
    ```

5. Run `make` to build the project:
    ```bash
    make
    ```

## 3. UART Connection

- **UART2 RX**: GPIO EXPANSION 27
- **UART2 TX**: GPIO EXPANSION 28
- **GND**: GPIO EXPANSION 30


## 4. Memory Setup

1. Prepare a TF card with MBR and FAT32. In the root directory of the TF card, download and place the following firmware files:
    - `bcm2711-rpi-4-b.dtb`
    - `bootcode.bin`
    - `start4.elf`
    - copy `kernel8.img`

2. Create a `config.txt` file with the following content:
    ```text
    kernel=kernel8.img
    arm_64bit=1
    enable_uart=1
    uart_2ndstage=1
    ```

This setup will enable you to run the FreeRTOS UART-2 shell application on your Raspberry Pi 4B.


## Sample output
Hello World !!!                                                                                                                   
FreeRTOS command server.                                                                                                                                                               
Type Help to view a list of registered commands.                                                                                                                                       
                                                                                                                                                                                       
\> help                                                                                                                                                                                  
                                                                                                                                                                                       
help:                                                                                                                                                                                  
 Lists all the registered commands                                                                                                                                                     
                                                                                                                                                                                       
                                                                                                                                                                                       
task-stats:                                                                                                                                                                            
 Displays a table showing the state of each FreeRTOS task                                                                                                                              
                                                                                                                                                                                       
echo-3-parameters <param1> <param2> <param3>:                                                                                                                                          
 Expects three parameters, echos each in turn                                                                                                                                          
                                                                                                                                                                                       
echo-parameters <...>:                                                                                                                                                                 
 Take variable number of parameters, echos each in turn                                                                                                                                
                                                                                                                                                                                       
task-stats:                                                                                                                                                                            
 Displays a table showing the state of each FreeRTOS task                                                                                                                              
                                                                                                                                                                                       
echo-3-parameters <param1> <param2> <param3>:                                                                                                                                          
 Expects three parameters, echos each in turn                                                                                                                                          
                                                                                                                                                                                       
echo-parameters <...>:                                                                                                                                                                 
 Take variable number of parameters, echos each in turn                                                                                                                                
                                                                                                                                                                                       
[Press ENTER to execute the previous command again]                                                                                                                                                                                                  


