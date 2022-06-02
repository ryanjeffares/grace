import argparse
import os


parser = argparse.ArgumentParser(
    description='Build the Grace interpreter from source'
)
parser.add_argument(
    'config', type=str, help='Configuration (Release/Debug/All)'
)


def main(config: str):
    if not os.path.isdir('build'):
        os.mkdir('build')

    if config == 'Release' or config == 'Debug':
        print(f'INFO: Generating CMake project for configuration: {config}\n')
        os.system(f'cmake -DCMAKE_BUILD_TYPE={config} -S . -B build')
        print()
        print(f'INFO: Building configuration: {config}\n')
        os.system(f'cmake --build build --config {config} -- -j8')
    elif config == 'All':
        print('INFO: Generating CMake project for configuration: Debug\n')
        os.system('cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build')
        print()
        print('INFO: Building configuration: Debug\n')
        os.system('cmake --build build --config Debug -- -j8')

        print()

        print('INFO: Generating CMake project for configuration: Release\n')
        os.system('cmake -DCMAKE_BUILD_TYPE=Release -S . -B build')
        print()
        print('INFO: Building configuration: Release\n')
        os.system('cmake --build build --config Release -- -j8')
    else:
        raise ValueError('config must match "Debug" or "Release" or "All"')


if __name__ == "__main__":
    args = parser.parse_args()
    main(args.config)
