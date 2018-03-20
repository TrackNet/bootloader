# The Bootloader

TrackNet is excited to contribute to the LoRaWAN™ ecosystem by open-sourcing
the firmware updates over-the-air (FUOTA) portion of its end-device LoRaWAN stack.

One important component of this stack is an update-capable bootloader. This loader
supports the unpacking and installing of new firmware on an end-device.

Over the next few days, we will start migrating our internal codebase to this
public repository, where we will then continue to evolve and maintain this component.
This file will be updated as this process continues.

## Update 1
*Tue, Mar 20 2018*

The bootloader skeleton and build environment have been migrated to this new
repository. We recommend the use of a recent Ubuntu distribution as build
host. We use `gcc-arm-embedded` from this PPA:
<https://launchpad.net/~team-gcc-arm-embedded/+archive/ubuntu/ppa>

To build, change into the target board's build directory and type make:

```
cd build/boards/B-L072Z-LRWAN1
make
```

You'll end up with a file called `bootloader.hex` that can be loaded onto the
B-L072Z-LRWAN1 STM32 LoRa™ Discovery kit. Since there is no valid firmware yet,
the LED LD2 will be flashing the corresponding error sequence (SYNC-2-2-1).
