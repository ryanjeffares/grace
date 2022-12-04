import os

if __name__ == "__main__":
    if not os.path.isdir('build'):
        os.mkdir('build')

    print('INFO: Generating CMake project\n')
    os.system('cmake -DGRACE_BUILD_TARGET=exe-DCMAKE_BUILD_TYPE=Release -S . -B build')
    print()
    print('INFO: Building configuration\n')

    install_status = os.system('cmake --build build --config Release --target install')
    if install_status == 0:
        print("\n\n=== INSTALLATION SUCCESSFUL ===\n\n")
        if os.name == 'nt':
            print("To use grace, add grace's installation directory to your PATH and set the GRACE_STD_PATH environment variable. This is usually C:/Program Files (x86)/grace/bin and C:/Program Files (x86)/grace/std, but check the above log to see if it was different.")
        else:
            print("To use grace, set the GRACE_STD_PATH environment variable in your shell config. This will be /usr/local/share/grace/std")
