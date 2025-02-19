# A9NLoader

A9NLoader is a bootloader designed to launch the [*A9N Microkernel*](https://github.com/horizon2038/A9N) in accordance with the `A9N Boot Protocol (x86_64)`. It properly loads the Kernel Executable and Init Executable, making the system operational.  
It is implemented using [*EDK2*](https://github.com/tianocore/edk2).

## Dependencies

- cmake
- clang-19
- lld-19
- llvm-19
- build-essential
- uuid-dev
- iasl
- git
- nasm 
- python-is-python3

## Build

```bash
mkdir build
cmake -B build
cmake --build build
```

## Build with Docker (Recommended)

```bash
TODO
```

## License

[MIT License](https://choosealicense.com/licenses/mit/)
