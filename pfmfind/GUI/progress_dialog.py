#
# Copyright (C) 2004-2006 Victoria University of Wellington
#
# This file is part of the PFMFind module.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2,
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#


from progress_bar import ProgressBar
import Tkinter
import Pmw
import threading

class ProgressDialog(Pmw.Dialog):
    def __init__(self, parent=None, **args):
        th = args.pop('toplevel_height', 200)
        tw = args.pop('toplevel_width', 400)
        ac = args.pop('activatecommand', None)
        self.no_bar = args.pop('nobar', False)
    
        Pmw.Dialog.__init__(self, parent, buttons=[],
                            activatecommand = ac)
        self.component('hull').overrideredirect(1)
        w = self.interior()
        w.pack_propagate(0)
        w.configure(bd=5, relief='raised', height=th, width=tw)
        self.text = args.pop('text', 'Progress bar')
        self.text_var = Tkinter.StringVar()
        self.text_var.set(self.text)
        Tkinter.Label(w, textvariable=self.text_var).pack(fill='y', expand=1)
        if not self.no_bar:
            self.pb = ProgressBar(w, **args)
            self.pb.pack(side='top', expand=1, anchor='n')
            self.counter = 0
            self.pb.updateProgress(self.counter)

    def incr(self):
        self.counter += 1
        self.pb.updateProgress(self.counter)

    def set(self, value, max=None):
        self.counter = value
        self.pb.updateProgress(self.counter, max)

    def message(self, text):
        self.text = text
        self.text_var.set(self.text)


class ActionProgress(ProgressDialog):
    def __init__(self, parent=None, actions=[], **pdargs):
        pdargs['nobar'] = True
        pdargs['activatecommand'] = self.activatecommand
        ProgressDialog.__init__(self, parent, **pdargs)
        self.actions = actions
        self.parent = parent
        self.activate(geometry = 'centerscreenalways')

    def activatecommand(self):
        try:
            for acts in self.actions:
                self.message(acts[0])
                self.update()
                f = acts[1]
                f(*acts[2])
        finally:
            self.deactivate()

class ThreadedAction(ProgressDialog):
    def __init__(self, parent=None, target=None, args=(),
                 kwargs={}, **pdargs):
        
        pdargs['nobar'] = True
        pdargs['activatecommand'] = self.activatecommand
        ProgressDialog.__init__(self, parent, **pdargs)

        self.target = target
        self.kwargs = kwargs
        self.args = args

        self.activate(geometry = 'centerscreenalways')
        
    def activatecommand(self):
        self.update()
        self.target(*self.args, **self.kwargs)
        #t = threading.Thread(target=self.target,
        #                     args = self.args,
        #                     kwargs = self.kwargs)
        #t.start()
        #t.join()
        self.deactivate()

if __name__ == '__main__':
    root = Pmw.initialise()
    ProgressDialog(width=300, height=30)
    root.mainloop()
