import argparse
import io
import sys
import tkinter
from tkinter import scrolledtext

from lang.translator import Translator


class MainWindow(tkinter.Tk):
    def __init__(self):
        tkinter.Tk.__init__(self)
        self.title("Figure 7.38")
        self.geometry("640x480")

        # Add layout for buttons and editor
        self._frame = tkinter.Frame(self)
        self._frame.grid_configure(row=2, column=1)
        self._frame.pack(fill=tkinter.BOTH, expand=True)

        # Add text editor
        self.editor = tkinter.scrolledtext.ScrolledText(self._frame)
        self.editor.pack(fill=tkinter.BOTH, expand=True)

        # Add "Translate" button
        self.translate = tkinter.Button(
            self._frame, text="Translate", command=self.translate_clicked
        )
        self.translate.pack()

    def translate_clicked(self):
        # Read from 1st row, 0th character to the end of the text box
        text = self.editor.get("1.0", tkinter.END).rstrip()
        tr = Translator(io.StringIO(text + "\n"))
        tr.translate(sys.stdout)


def main():
    # Call with no arguments to open the GUI, or with a single file argument to run in the terminal.
    parser = argparse.ArgumentParser(description="Fig 07.38")
    parser.add_argument("input_file", default=None, nargs="?")
    args = parser.parse_args()
    if args.input_file:
        with open(args.input_file, "r") as f:
            tr = Translator(io.StringIO("".join(f.readlines()) + "\n"))
            tr.translate(sys.stdout)
    else:
        # No passed file, so set up main window, and run until explicitly closed.
        window = MainWindow()
        window.mainloop()


if __name__ == "__main__":
    main()
