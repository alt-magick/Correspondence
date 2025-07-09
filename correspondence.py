#!/usr/bin/env python3
import tkinter as tk
from tkinter import filedialog, messagebox, font as tkfont
import sys

class CorrespondenceApp:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Correspondence")
        self.root.geometry("480x640")
        
        # Position window on the right side
        self.root.update_idletasks()
        screen_width = self.root.winfo_screenwidth()
        screen_height = self.root.winfo_screenheight()
        x = screen_width - 500
        y = (screen_height - 640) // 2
        self.root.geometry(f"480x640+{x}+{y}")
        
        self.enable_substitution = False
        self.undo_stack = []
        self.redo_stack = []
        
        # Character mapping
        self.char_map = {
            'a': '☉', 'b': '●', 'c': '☾', 'd': '☽', 'e': '○',
            'f': '☿', 'g': '♀', 'h': '♁', 'i': '♂', 'j': '♃',
            'k': '♄', 'l': '♅', 'm': '♆', 'n': '♇', 'o': '♈',
            'p': '♉', 'q': '♊', 'r': '♋', 's': '♌', 't': '♍',
            'u': '♎', 'v': '♏', 'w': '♐', 'x': '♑', 'y': '♒', 'z': '♓'
        }
        
        # Reverse mapping for decoding
        self.reverse_map = {v: k for k, v in self.char_map.items()}
        
        self.setup_ui()
        self.setup_bindings()
        self.update_status()
    
    def setup_ui(self):
        # Create menu
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        # File menu
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="Open - Ctrl+O", command=self.open_file)
        file_menu.add_command(label="Save - Ctrl+S", command=self.save_file)
        file_menu.add_separator()
        file_menu.add_command(label="Exit - Ctrl+Q", command=self.exit_app)
        
        # Edit menu
        edit_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Edit", menu=edit_menu)
        edit_menu.add_command(label="Undo - Ctrl+Z", command=self.undo)
        edit_menu.add_command(label="Redo - Ctrl+Y", command=self.redo)
        edit_menu.add_separator()
        edit_menu.add_command(label="Cut - Ctrl+X", command=self.cut)
        edit_menu.add_command(label="Copy - Ctrl+C", command=self.copy)
        edit_menu.add_command(label="Paste - Ctrl+V", command=self.paste)
        edit_menu.add_separator()
        edit_menu.add_command(label="Select All - Ctrl+A", command=self.select_all)
        
        # Code menu
        code_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Code", menu=code_menu)
        code_menu.add_command(label="Encode - Ctrl+E", command=self.encode_text)
        code_menu.add_command(label="Decode - Ctrl+D", command=self.decode_text)
        code_menu.add_command(label="Toggle - Ctrl+T", command=self.toggle_substitution)
        
        # Main frame
        main_frame = tk.Frame(self.root)
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # Text widget with scrollbar
        text_frame = tk.Frame(main_frame)
        text_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        self.text_widget = tk.Text(text_frame, wrap=tk.WORD, undo=True)
        scrollbar = tk.Scrollbar(text_frame, orient=tk.VERTICAL, command=self.text_widget.yview)
        self.text_widget.configure(yscrollcommand=scrollbar.set)
        
        # Configure font for emoji support
        try:
            emoji_font = tkfont.Font(family="Noto Color Emoji", size=14)
            self.text_widget.configure(font=emoji_font)
        except:
            # Fallback to system font
            self.text_widget.configure(font=("TkDefaultFont", 14))
        
        self.text_widget.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Status bar
        self.status_var = tk.StringVar()
        self.status_bar = tk.Label(self.root, textvariable=self.status_var, 
                                  relief=tk.SUNKEN, anchor=tk.W)
        self.status_bar.pack(side=tk.BOTTOM, fill=tk.X)
    
    def setup_bindings(self):
        # Keyboard shortcuts
        self.root.bind('<Control-o>', lambda e: self.open_file())
        self.root.bind('<Control-s>', lambda e: self.save_file())
        self.root.bind('<Control-q>', lambda e: self.exit_app())
        self.root.bind('<Control-e>', lambda e: self.encode_text())
        self.root.bind('<Control-d>', lambda e: self.decode_text())
        self.root.bind('<Control-t>', lambda e: self.toggle_substitution())
        self.root.bind('<Control-z>', lambda e: self.undo())
        self.root.bind('<Control-y>', lambda e: self.redo())
        self.root.bind('<Control-x>', lambda e: self.cut())
        self.root.bind('<Control-c>', lambda e: self.copy())
        self.root.bind('<Control-v>', lambda e: self.paste())
        self.root.bind('<Control-a>', lambda e: self.select_all())
        
        # Character substitution
        self.text_widget.bind('<KeyPress>', self.on_key_press)
        self.text_widget.bind('<Key>', self.on_text_change)
        
        # Window close
        self.root.protocol("WM_DELETE_WINDOW", self.exit_app)
    
    def save_state_for_undo(self):
        current_text = self.text_widget.get("1.0", tk.END)
        self.undo_stack.append(current_text)
        self.redo_stack.clear()
        
        # Limit undo stack size
        if len(self.undo_stack) > 50:
            self.undo_stack.pop(0)
    
    def update_status(self):
        status = f"Code {'Enabled' if self.enable_substitution else 'Disabled'}"
        self.status_var.set(status)
    
    def on_key_press(self, event):
        # Handle real-time substitution
        if (self.enable_substitution and 
            len(event.char) == 1 and 
            event.char.lower() in self.char_map and
            not (event.state & 0x4)):  # Not Ctrl pressed
            
            # Insert the symbol instead of the character
            symbol = self.char_map[event.char.lower()]
            self.text_widget.insert(tk.INSERT, symbol)
            return "break"  # Prevent default behavior
        
        return None
    
    def on_text_change(self, event):
        # Save state for certain keys
        if event.keysym in ['Return', 'BackSpace', 'Delete']:
            self.save_state_for_undo()
    
    def encode_text(self):
        self.save_state_for_undo()
        
        content = self.text_widget.get("1.0", tk.END)
        encoded = ""
        
        for char in content:
            if char.lower() in self.char_map:
                encoded += self.char_map[char.lower()]
            else:
                encoded += char
        
        self.text_widget.delete("1.0", tk.END)
        self.text_widget.insert("1.0", encoded)
        self.update_status()
    
    def decode_text(self):
        self.save_state_for_undo()
        
        content = self.text_widget.get("1.0", tk.END)
        decoded = ""
        
        i = 0
        while i < len(content):
            found = False
            # Check for multi-byte Unicode symbols
            for symbol, letter in self.reverse_map.items():
                if content[i:].startswith(symbol):
                    decoded += letter
                    i += len(symbol)
                    found = True
                    break
            
            if not found:
                decoded += content[i]
                i += 1
        
        self.text_widget.delete("1.0", tk.END)
        self.text_widget.insert("1.0", decoded)
        self.update_status()
    
    def toggle_substitution(self):
        self.enable_substitution = not self.enable_substitution
        self.update_status()
    
    def undo(self):
        if self.undo_stack:
            current_text = self.text_widget.get("1.0", tk.END)
            self.redo_stack.append(current_text)
            
            previous_text = self.undo_stack.pop()
            self.text_widget.delete("1.0", tk.END)
            self.text_widget.insert("1.0", previous_text)
    
    def redo(self):
        if self.redo_stack:
            current_text = self.text_widget.get("1.0", tk.END)
            self.undo_stack.append(current_text)
            
            next_text = self.redo_stack.pop()
            self.text_widget.delete("1.0", tk.END)
            self.text_widget.insert("1.0", next_text)
    
    def cut(self):
        try:
            self.text_widget.event_generate("<<Cut>>")
        except:
            pass
    
    def copy(self):
        try:
            self.text_widget.event_generate("<<Copy>>")
        except:
            pass
    
    def paste(self):
        try:
            self.text_widget.event_generate("<<Paste>>")
        except:
            pass
    
    def select_all(self):
        self.text_widget.tag_add(tk.SEL, "1.0", tk.END)
        self.text_widget.mark_set(tk.INSERT, "1.0")
        self.text_widget.see(tk.INSERT)
    
    def save_file(self):
        filename = filedialog.asksaveasfilename(
            defaultextension=".txt",
            filetypes=[("Text files", "*.txt"), ("All files", "*.*")]
        )
        
        if filename:
            try:
                content = self.text_widget.get("1.0", tk.END)
                with open(filename, 'w', encoding='utf-8') as f:
                    f.write(content.rstrip('\n'))  # Remove trailing newline
                
                messagebox.showinfo("Correspondence", "File saved successfully!")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to save file: {str(e)}")
    
    def open_file(self):
        filename = filedialog.askopenfilename(
            filetypes=[("Text files", "*.txt"), ("All files", "*.*")]
        )
        
        if filename:
            try:
                with open(filename, 'r', encoding='utf-8') as f:
                    content = f.read()
                
                self.text_widget.delete("1.0", tk.END)
                self.text_widget.insert("1.0", content)
            except Exception as e:
                messagebox.showerror("Error", f"Failed to open file: {str(e)}")
    
    def exit_app(self):
        self.root.quit()
        self.root.destroy()
        sys.exit(0)
    
    def run(self):
        self.root.mainloop()

def main():
    app = CorrespondenceApp()
    app.run()

if __name__ == "__main__":
    main()
