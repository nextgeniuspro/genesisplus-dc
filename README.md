# Genesis Plus DC
Sega Genesis/Megadrive emulator for the Sega Dreamcast console.

## Description

Genesis Plus DC is a Sega Genesis/Megadrive emulator for the Sega Dreamcast console. 
                                                    
## Building

To build Genesis Plus DC:

```
cd dreamcast-x
make all
``` 

## Creating CDI

To create a CDI image to run on Dreamcast:

```
make genplus-x.bin
make 1st_read.bin 
make selfboot
```
                                                    
CDI image will be created in the `disc` directory.