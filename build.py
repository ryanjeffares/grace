import argparse
import os


parser = argparse.ArgumentParser(
    description='Build the Grace interpreter from source'
)
parser.add_argument('config', type=str, help='Configuration (Release/Debug)')


def main(config: str):
    if not os.path.isdir('build'):
        os.mkdir('build')

    print('INFO: Generating cmake project\n')

    if config != 'Release' and config != 'Debug':
        raise ValueError('config must match "Debug" or "Release"')

    os.system(f'cmake -DCMAKE_BUILD_TYPE={config} -S . -B build')

    print()
    print('INFO: Building\n')

    os.system('cmake --build build')


if __name__ == "__main__":
    args = parser.parse_args()
    main(args.config)
