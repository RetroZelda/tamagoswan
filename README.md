# TamagoSwan
TamaLib implementation in the Bandai WonderSwan Color utilizing the Wonderful Toolchain.

This is an educational project for me to learn about Wonderswan developement.  
Expect terrible performance of Tamalib, but feel free to contribute to either this repo or the Tamalib fork to increase performance. 



<img width="237" height="144" alt="tamagoswan_002" src="https://github.com/user-attachments/assets/7971d444-b075-4579-9188-2f060c6f0cc1" />
<img width="237" height="144" alt="tamagoswan_001" src="https://github.com/user-attachments/assets/c10eb82c-714d-48b5-aa3b-bc8306482f36" />
<img width="237" height="144" alt="tamagoswan_000" src="https://github.com/user-attachments/assets/788103af-c548-47c9-a0f8-c34bf8ad72c2" />
<br>
<img width="237" height="144" alt="tamagoswan_003" src="https://github.com/user-attachments/assets/b717ee34-265e-48b2-990c-015e3f04ea55" />
<img width="237" height="144" alt="tamagoswan_005" src="https://github.com/user-attachments/assets/74823119-d3d0-4130-b01f-c3b130e15002" />
<img width="237" height="144" alt="tamagoswan_007" src="https://github.com/user-attachments/assets/c76d09a6-80b9-4572-88a0-538a6cbb3144" />

This was tested with the P1 rom, however any tamalib compatible rom should work.  The roms need their endianness flipped manually before including in the build.

Requires WonderSwan Color or Crystal. Tested on Mesen2 but untested on actual hardware.  Let me know if you have a flashcart you want to sell :)

## Controlls
- X4 - Left Button
- X3 - Middle Button
- X2 - Right Button

Due to poor performance, you may need to hold buttons down in order for them to get properly handled by Tamalib

## Notes

You need to get my TamaLib fork that includes various optimizations and configuration changes specific to the wonderswan. 

The fork exists here: https://github.com/RetroZelda/tamalib but it is also setup as a submodule in this repository.


## Links

Wonderful Toolchain: https://wonderful.asie.pl/

TamaLib: https://github.com/jcrona/tamalib
