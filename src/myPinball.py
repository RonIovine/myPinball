#!/usr/bin/python
import Tkinter as tk
#import ttk

# dummy variables so we can create pseudo end block indicators, add these identifiers to your
# list of python keywords in your editor to get syntax highlighting on these identifiers
enddef = endif = endwhile = endfor = endtry = endexcept = endclass = None

# python does not have a native null string identifier, so create one
NULL = ""

class Application(tk.Frame):        
  def __init__(self, master=None):
    tk.Frame.__init__(self, master)
    self.grid()                    
    self.createWidgets()
  enddef

  def createWidgets(self):
    self.quitButton = tk.Button(self, text='Quit', command=self.quit) 
    self.quitButton.grid()    
    self.okButton = tk.Button(self, text='Ok') 
    self.okButton.grid()    
    self.cancelButton = tk.Button(self, text='Cancel') 
    self.cancelButton.grid()
    self.cancelButton.bind('<Button-1>', self.leftMouse)
  enddef

  def leftMouse(self, event):
    print "Left mouse clicked"
  enddef
    
endclass
            
app = Application()                     
app.master.title('MyPinball Management')  
app.mainloop()      
