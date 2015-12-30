# Architecture

Computer Organization (CS311) by Jaehyuk Huh, KAIST

## Lab 1: MIPS assembler

```
python lab1/assembler.py <assembly file>
```

When the assembly file is `lab1/CS311/example_mod.s`, then the output should be
the same as `lab1/CS311/example_mod.o`.

## Lab 2: Building a Simple MIPS Emulator

```
make
./cs311em [-m addr1:addr2] [-d] [-n num_instr] inputBinary
```

Test with `./test.sh`.

## Lab 3: Simulating Pipelined Execution

```
make
./cs311sim [-nobp] [-f] [-m addr1:addr2] [-d] [-p] [-n num_instr] inputBinary
```

Test with `./test.sh`.

## Lab 4: Cache Design for MIPS Architecture

```
gcc cache_output_format.c -o cs311cache
./cs311cache [-c cap:assoc:bsize] [-x] input_trace
```

Test with `./test.sh`.
