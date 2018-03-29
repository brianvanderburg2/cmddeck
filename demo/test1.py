import serial

class MySerial(serial.Serial):
    def write(self, what):
        print(what.rstrip())
        serial.Serial.write(self, what)

class App(object):
    
    def __init__(self, port):
        self._port = port
        self._page_stack = []
        self._p = None
        self._lp = None

    def run(self):
        # Connect and display initial messages
        s = self._s = MySerial(self._port, 9600, timeout=None)
        self.dump(5)

        # Run the loop
        while self._p:
            if self._p != self._lp:
                self._p.display()
                self._lp = self._p

            line = s.readline().rstrip()
            try:
                value = int(line)
                self._p.on_button(value)
            except ValueError:
                print(line.rstrip())

    def push_page(self, page):
        self._page_stack.append(page)
        self._p = page

    def pop_page(self, n=1):
        self._page_stack = self._page_stack[:-n]
        if self._page_stack:
            self._p = self._page_stack[-1]
        else:
            self._p = None
            
    def change_page(self, page):
        p = self._page_stack.pop()
        self._page_stack.append(page)
        self._p = page
        return p

    def dump(self, timeout=0.1):
        s = self._s
        s.timeout = timeout
        line = s.readline()
        while(line):
            print(line.rstrip())
            line = s.readline()

        s.timeout = None

    @classmethod
    def execute(cls, pgcls, port):
        app = cls(port)
        page = pgcls(app)
        app.push_page(page)
        app.run()

class Page(object):

    def __init__(self, app):
        self._a = app
        self._buttons = []

        self.add_buttons()

    def add_buttons(self):
        pass

    def display(self):
        ## Redraw the page
        s = self._a._s

        s.write("freeze\n")
        s.readline()
        s.write("clearbuttons\n".encode("utf8"))
        s.readline()
        for i, b in enumerate(self._buttons):
            s.write("addbutton {0}{1}{2}{3}\n".format(b.x, b.y, b.w, b.h).encode("utf8"))
            s.readline()
            self.set_image(i, b.filename)
        s.write("unfreeze\n".encode("utf8"))
        s.readline()
        self._a.dump()

    def add_button(self, x, y, w, h, filename="", callback=None):
        self._buttons.append(Button(x, y, w, h, filename, callback))
        self._buttons[-1].index = len(self._buttons) - 1

    def set_image(self, i, filename):
        s = self._a._s

        b = self._buttons[i]
        b.filename = filename
        s.write("button{} {}\n".format(i + 1, b.filename).encode("utf8"))
        s.readline()

    def on_button(self, i):
        i = i - 1
        if i >= 0 and i < len(self._buttons):
            cb = self._buttons[i].callback
            if cb:
                cb(self._a, self._buttons[i])

class Button(object):

    def __init__(self, x, y, w=1, h=1, filename="", callback=None):
        self.x = x
        self.y = y
        self.w = w
        self.h = h
        self.filename = filename
        self.callback = callback


class MyStartPage(Page):

    def add_buttons(self):
        self.add_button(1, 1, 2, 2, "0001", self._button1)
        self.add_button(1, 3, 2, 2, "0002", self._button2)

    def _button1(self, app, button):
        self.set_image(button.index, "0001")
        p = MyExitPage(self._a)
        p._orig = self._a.change_page(p)

    def _button2(self, app, button):
        self.set_image(button.index, "0004")

        p = OtherPage(self._a)
        self._a.push_page(p)

class MyExitPage(Page):
    def add_buttons(self):
        self.add_button(1, 1, 2, 2, "0001", self._yes)
        self.add_button(4, 1, 2, 2, "0002", self._no)

    def _yes(self, app, button):
        self._a.pop_page()

    def _no(self, app, button):
        self._a.change_page(self._orig)

class OtherPage(Page):

    def add_buttons(self):
        self.add_button(1, 1, 5, 1, "0009", self._button1)

    def _button1(self, app, button):
        app.pop_page()


App.execute(MyStartPage, "/dev/ttyACM0")

