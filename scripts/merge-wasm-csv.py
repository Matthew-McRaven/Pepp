import glob, sys, shutil
with open(sys.argv[1], 'wb') as outfile:
  for filename in glob.glob(f'{sys.argv[2]}/*.txt'):
    with open(filename, 'rb') as readfile:
      shutil.copyfileobj(readfile, outfile)
