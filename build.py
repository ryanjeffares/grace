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
            install_status = os.system(f'cmake --build build --config {config} --target install')
            if install_status == 0:
                print("\n\n=== INSTALLATION SUCCESSFUL ===\n\n")
                if os.name == 'nt':
                    print("To use grace, add grace's installation directory to your PATH and set the GRACE_STD_PATH environment variable. This is usually C:/Program Files (x86)/grace/bin and C:/Program Files (x86)/grace/std, but check the above log to see if it was different.")
                else:
                    print("To use grace, set the GRACE_STD_PATH environment variable in your shell config. This will be /usr/local/share/grace/std")
        else:
            os.system(f'cmake --build build --config {config}')        
