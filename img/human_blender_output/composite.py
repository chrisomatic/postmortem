from time import time, sleep
from random import choice
from datetime import datetime
import sys, os
import math
import struct
from PyQt5.QtWidgets import *
from PyQt5.QtGui import QKeyEvent, QPainter,QImage, QPen, QIcon, QPixmap, QColor, QBrush, QCursor, QFont, QPalette, QTransform, QLinearGradient, QFontMetrics, QStaticText, qRgba
from PyQt5.QtCore import Qt, QPoint, QPointF, QSize, QEvent, QTimer, QCoreApplication, QRect
import copy
import numpy as np


slash = "/"
if sys.platform == "win32":
    slash = "\\"


def bound(_val, _min, _max):
    return max(min(_val, _max), _min)

def printf(fmt, *args):
    if(len(args) > 0):
        print(fmt % args, end="")
    else:
        print(fmt, end="")


class Compositor(QWidget):
    def __init__(self, parent=None):
        # QMainWindow.__init__(self, parent)
        super().__init__()


        
        self.color_clear = QColor(0,0,0,0)
        self.color_rep_clear = QColor(255,0,255,255)
        # self.color_rep_clear = QColor(0,0,0,255)

        # self.color_clear = qRgba(0,0,0,0)
        # self.color_rep_clear = QColor(0,0,0,255)

        self.root = root = os.path.dirname(os.path.abspath(__file__)) + slash

        #self.path = self.root + "standing" + slash
        #self.path = self.root + "walk1" + slash
        self.path = self.root + "walk2" + slash

        self.scaled_size = QSize(64, 128)

        pics = [x.lower() for x in os.listdir(self.path) if os.path.isfile(self.path+x) and x.lower().endswith(".png") and x.lower().startswith("000")]
        pics.sort()

        if(len(pics) == 0):
            printf("No images found")
            QCoreApplication.instance().quit()

        # pics = [pics[0]]

        self.pic_data = {}
        for i in range(len(pics)):
            p = pics[i]
            # printf("Loading image %s (%d of %d)      \r", p, i+1, len(pics))
            printf("Loading image %s (%d of %d)      \n", p, i+1, len(pics))
            num = int(p.replace(".png",""))
            img = QImage(self.path+p,"PNG")
            img = img.convertToFormat(QImage.Format_ARGB32)
            img,cnt = self.sub_img_color(img, self.color_rep_clear, self.color_clear)
            w = img.width()
            h = img.height()

            # https://doc.qt.io/qtforpython-5/PySide2/QtGui/QImage.html?highlight=qimage#PySide2.QtGui.PySide2.QtGui.QImage.scaled
            # https://doc.qt.io/qtforpython-5/PySide2/QtCore/Qt.html#PySide2.QtCore.PySide2.QtCore.Qt.AspectRatioMode
            img_scaled = img.scaled(self.scaled_size, Qt.IgnoreAspectRatio, Qt.SmoothTransformation)

            self.pic_data[p] = {}
            self.pic_data[p]["num"] = num
            self.pic_data[p]["img"] = img
            self.pic_data[p]["img_scaled"] = img_scaled

        # printf("Loaded %d image(s)                                                 \n", len(self.pic_data))

        self.keys = [x for x in self.pic_data.keys()]
        self.krange = range(len(self.keys))

        self.draw_png()
        QCoreApplication.instance().quit()

    def sub_img_color(self, img, find, replace):

        am = img.createAlphaMask()
        # am.fill(1)
        am.fill(0)
        ptr = img.bits()
        ptr.setsize(img.byteCount())
        ## copy the data out as a string
        strData = ptr.asstring()
        ## get a read-only buffer to access the data
        buf = memoryview(ptr)
        ## view the data as a read-only numpy array
        arr = np.frombuffer(buf, dtype=np.ubyte).reshape(img.height(), img.width(), 4)
        ## view the data as a writable numpy array
        arr = np.asarray(ptr).reshape(img.height(), img.width(), 4)


        count = 0
        for y in range(img.height()):
            for x in range(img.width()):
                color = QColor(img.pixel(x,y))
                if(color == find):
                    # img.setPixelColor(x,y,replace)
                    arr[y][x][3] = 0
                    color = QColor(img.pixel(x,y))
                    count += 1
        # img.setAlphaChannel(am)
        return img,count

    def draw_png(self):
        # return
        # print("draw png")
        tw = 0
        for i in self.krange:
            key = self.keys[i]
            tw += self.pic_data[key]["img_scaled"].width()

        image = QImage(tw, self.scaled_size.height(), QImage.Format_ARGB32)

        # painter = QPainter(image)

        painter = QPainter()
        painter.begin(image)

        # painter.setCompositionMode(QPainter.CompositionMode_Source)

        x = 0
        for i in self.krange:
            key = self.keys[i]
            img = self.pic_data[key]["img_scaled"]
            painter.drawImage(x,0,img)
            x += img.width()

        painter.end()
        path = self.path+"composite_test.png"
        image.save(path,"PNG")
        printf("Saved: %s\n", path)

    def paintEvent(self, event):
        print("paint event")
        self.draw_png()
        QCoreApplication.instance().quit()




if __name__ == "__main__":
    app = QApplication(sys.argv)
    c = Compositor()
    # w = MainWindow()
    # w.show()
    # # app.setQuitOnLastWindowClosed(False)
    # app.exec_()

