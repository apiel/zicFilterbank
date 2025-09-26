# Zic Filterbank

**Zic Filterbank** is a custom filterbank-to-FX box built with Daisy Seed, slightly inspired by the Sherman Filterbank, but very different in design.

Instead of boosting frequency bands like a traditional filterbank, this project applies **different audio effects** (distortion, drive, compression, bitcrusher, etc.) to selected frequency ranges while keeping the rest of the signal clean. On top of the band FX, there’s also a master FX section.

The goal is to add character and punch to drum machines (like the Korg Electribe ER-1) or any other audio source, while keeping control over how much of the signal is affected.

<img src='https://github.com/apiel/zicFilterbank/blob/main/assets/filterbank.png?raw=true'>

<table>
<tr>
<td>
<img src='https://github.com/apiel/zicFilterbank/blob/main/assets/filterbank1.png?raw=true' width='400'>
</td>
<td>
<img src='https://github.com/apiel/zicFilterbank/blob/main/assets/filterbank2.png?raw=true' width='400'>
</td>
</tr>
<table>

## ✨ Features

- Band FX processing (only affect chosen frequency ranges): **3 chained FX + Multimode resonant filter**
- Master FX section on top of band FX: **2 chained FX**
- Multiple FX types:
  - Drive, compression, clipping
  - Bitcrusher, sample reducer, decimator
  - Tremolo, ring modulation, inverter
  - Filters (low-pass, high-pass, distorted HPF)
  - multiple Reverbs and Delays

Built on Daisy Seed using DaisySP and libDaisy.

## 🚀 Getting Started
Clone the repository (with submodules)
```sh
git clone --recurse-submodules https://github.com/apiel/zicFilterbank.git
cd zicFilterbank
```

Build dependencies (first time only)
```sh
cd libDaisy
make clean && make
cd ../DaisySP
make clean && make
cd ..
```

Build the project
```
make
```

Upload to Daisy Seed (Seed must be in DFU bootloader mode)
```
make program-dfu
```

For detailed setup instructions, check the [official Daisy documentation](https://electro-smith.github.io/libDaisy/).

## PCB

The PCB has been designed using EasyEDA. To access the project, use the following link:

https://easyeda.com/editor#id=a471f378a96b47019cbb56552ec53f15

<img src='https://github.com/apiel/zicFilterbank/blob/main/assets/filterbank_pcb.png?raw=true' width='700'>

https://easyeda.com/editor#id=ba950a98b9194fd0b9de4e008d2ef155

<img src='https://github.com/apiel/zicFilterbank/blob/main/assets/filterbank_pcb_top.png?raw=true' width='700'>
