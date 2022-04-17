import os


for root, dirs, files in os.walk('./examples', topdown=True):
    for file in files:
        example_path = os.path.join(root, file)
        print('Running', example_path)
        os.system(f'./build/grace {example_path}')
