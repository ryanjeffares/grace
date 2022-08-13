import argparse
import os

parser = argparse.ArgumentParser(
    description='Build the Grace interpreter from source'
)
parser.add_argument(
    'config', type=str, help='Configuration (Release/Debug/All)'
)
parser.add_argument(
    '--install', action='store_true', required=False
)


def main(config: str, install: bool):
    if not os.path.isdir('build'):
        os.mkdir('build')

    if config == 'Release' or config == 'Debug':
        print(f'INFO: Generating CMake project for configuration: {config}\n')
        os.system(f'cmake -DCMAKE_BUILD_TYPE={config} -S . -B build')
        print()
        print(f'INFO: Building configuration: {config}\n')
        if os.name == 'nt':
            if install:
                os.system(f'cmake --build build --config {config} --target install')
            else:
                os.system(f'cmake --build build --config {config}')
        else:
            if install:
                os.system(f'cmake --build build --config {config} --target install -- -j')
            else:
                os.system(f'cmake --build build --config {config} -- -j')
    elif config == 'All':
        print('INFO: Generating CMake project for configuration: Debug\n')
        os.system('cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build')
        print()
        print('INFO: Building configuration: Debug\n')

        if os.name == 'nt':
            if install:
                os.system(f'cmake --build build --config Debug --target install')
            else:
                os.system(f'cmake --build build --config Debug')
        else:
            if install:
                os.system(f'cmake --build build --config Debug --target install -- -j')
            else:
                os.system(f'cmake --build build --config Debug -- -j')

        print()

        print('INFO: Generating CMake project for configuration: Release\n')
        os.system('cmake -DCMAKE_BUILD_TYPE=Release -S . -B build')
        print()
        print('INFO: Building configuration: Release\n')

        if os.name == 'nt':
            if install:
                os.system(f'cmake --build build --config Release --target install')
            else:
                os.system(f'cmake --build build --config Release')
        else:
            if install:
                os.system(f'cmake --build build --config Release --target install -- -j')
            else:
                os.system(f'cmake --build build --config Release -- -j')
    else:
        raise ValueError('config must match "Debug" or "Release" or "All"')


if __name__ == "__main__":
    args = parser.parse_args()
    main(args.config, args.install)
