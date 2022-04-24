import argparse
import os


parser = argparse.ArgumentParser(
    description='Build the Grace interpreter from source'
)
parser.add_argument('config', type=str, help='Configuration (release/debug)')


def main(config: str):
    if not os.path.isdir('build'):
        os.mkdir('build')

    if config == 'debug':
        os.system('cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build')
    elif config == 'release':
        os.system('cmake -DCMAKE_BUILD_TYPE=Release -S . -B build')
    else:
        raise ValueError('config must match "debug" or "release"')

    os.system('cmake --build build')


if __name__ == "__main__":
    args = parser.parse_args()
    main(args.config)
