You will notice that tests fail if you do no strip the final newline from each output file.
This is on purpose -- I want to match the output of the simulator exactly, and the simulator does not output \n magically on your behalf.

However, your text editor is liable to insert a final \n (or two!) so that the file ends in a newline.
Once you've identified the offending output use the following to strip the last byte of the file:
```sh
truncate -s -1 <book>/<chapter>/<figure>/io0/output.txt
```

Extra bytes on the input file do not matter -- if they are not read, they are silently ignored.
