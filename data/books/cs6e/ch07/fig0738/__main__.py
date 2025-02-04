import tkinter
from tkinter import scrolledtext, Toplevel


class MainWindow(tkinter.Toplevel):
    def __init__(self, master=None):
        Toplevel.__init__(self, master)
        self.title("Figure 7.38")
        self.geometry("640x480")

        # Add layout for buttons and editor
        self.frame = tkinter.Frame(self)
        self.frame.grid_configure(row=2, column=1)
        self.frame.pack(fill=tkinter.BOTH, expand=True)

        # Add text editor
        self.editor = tkinter.scrolledtext.ScrolledText(self.frame)
        self.editor.pack(fill=tkinter.BOTH, expand=True)

        # Add "Translate" button
        self.translate = tkinter.Button(
            self.frame, text="Translate", command=self.translate_clicked
        )
        self.translate.pack()

    def translate_clicked(self):
        # Read from 1st row, 0th character to the end of the text box
        text = self.editor.get("1.0", tkinter.END).rstrip()
        print("Will translate: ", text)


def main():
    # Set up main window, and run until explicitly closed.
    app = tkinter.Tk()
    window = MainWindow(app)
    window.mainloop()


if __name__ == "__main__":
    main()
