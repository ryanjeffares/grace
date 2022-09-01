import os
import time


def main():
    total = float(0)

    for i in range(0, 50):
        single_start = time.time()
        if os.name == 'nt':
            os.system("./build/grace/Release/Release/grace.exe ./examples/for.gr")
        else:
            os.system("./build/grace/Release/grace ./examples/for.gr")
        single_end = time.time()
        total += single_end - single_start

    average = total / 50

    print(f'Total: {total}, Average: {average}')


if __name__ == '__main__':
    main()
