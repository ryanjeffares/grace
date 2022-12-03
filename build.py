import argparse
import os

parser = argparse.ArgumentParser(
    description='Build the Grace interpreter from source'
)
parser.add_argument(
    'build_type', type=str, help='Build Type (exe/dll)'
)
parser.add_argument(
    'config', type=str, help='Configuration (Release/Debug/All)'
)
parser.add_argument(
    '--install', action='store_true', help='Install grace system-wide'
)


if __name__ == "__main__":
    args = parser.parse_args()
    build_type = args.build_type
    if build_type != "exe" and build_type != "dll":
        raise ValueError('build_type must match "exe" or "dll"')
    
    config = args.config
    if config == "Debug" or config == "Release":
        configs = [config]
    elif config == "All":
        configs = ["Debug", "Release"]
    else:
        raise ValueError('config must match "Debug" or "Release" or "All"')
    
    if not os.path.isdir('build'):
        os.mkdir('build')

    for config in configs:
        print(f'INFO: Generating CMake project for configuration: {config}\n')
        os.system(f'cmake -DGRACE_BUILD_TARGET={build_type} -DCMAKE_BUILD_TYPE={config} -S . -B build')
        print()
        print(f'INFO: Building configuration: {config}\n')
        
        if args.install:
            os.system(f'cmake --build build --config {config} --target install')
        else:
            os.system(f'cmake --build build --config {config}')        
