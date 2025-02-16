# A9NLoader

A9NLoader is the bootloader designed to launch the A9N Kernel.

## Usage

The A9NLoader obtains the memory map and places the `kernel.elf` appropriately in memory.
While it's primarily intended for booting the A9N Kernel, it's capable of launching other kernels in ELF format as well.
In such cases, ensure your kernel is designed to receive the address of the `boot_info` structure.

## Build

```bash
mkdir build
cmake -B build
cmake --build build
```

## License

[MIT License](https://choosealicense.com/licenses/mit/)
