import os
import time

for root, dirs, files in os.walk('./examples', topdown=True):
    for file in files:
        start = time.time()
        example_path = os.path.join(root, file)
        print('Running', example_path)
        print()
        if os.name == 'nt':
            os.system(f'./build/grace/Release/Release/grace.exe')
        else:
            os.system(f'./build/grace/Release/grace {example_path}')
        print()
        print(f'Finished in {time.time() - start}')
