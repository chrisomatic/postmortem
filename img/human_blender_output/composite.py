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

        self.scaled_size = QSize(96, 128)

        self.num_cols = 16

        nums = ["00%02d" % i for i in range(self.num_cols)]

        self.pic_data = {}

        self.folders = ["walk_normal", "walk_gun_ready"]
        self.orientations = ["front" , "front_right", "right", "back_right", "back", "back_left", "left", "front_left"]


        for i in range(len(self.folders)):
            # self.pic_data.append({})
            f = self.folders[i]
            fpath = self.root + f + slash

            for j in range(len(self.orientations)):
                o = self.orientations[j]

                search_pics = [n + "_" + o + ".png" for n in nums]

                pics = [x.lower() for x in os.listdir(fpath) if os.path.isfile(fpath+x) and x.lower() in search_pics]
                pics.sort()

                self.num_cols = len(pics)

                for p in pics:
                    self.pic_data[fpath+p] = {}


        self.keys = [x for x in self.pic_data.keys()]
        self.krange = range(len(self.keys))

        for i in self.krange:
            path = self.keys[i]

            # printf("Loading image %s (%d of %d)      \r", path, i+1, len(self.keys))
            printf("Loading image %d of %d      \r", i+1, len(self.keys))
            img = QImage(path,"PNG")
            img = img.convertToFormat(QImage.Format_ARGB32)
            # img,cnt = self.sub_img_color(img, self.color_rep_clear, self.color_clear)
            # w = img.width()
            # h = img.height()

            # https://doc.qt.io/qtforpython-5/PySide2/QtGui/QImage.html?highlight=qimage#PySide2.QtGui.PySide2.QtGui.QImage.scaled
            # https://doc.qt.io/qtforpython-5/PySide2/QtCore/Qt.html#PySide2.QtCore.PySide2.QtCore.Qt.AspectRatioMode
            img_scaled = img.scaled(self.scaled_size, Qt.IgnoreAspectRatio, Qt.SmoothTransformation)

            # self.pic_data[p] = {}
            # self.pic_data[p]["num"] = num
            self.pic_data[path]["img"] = img
            self.pic_data[path]["img_scaled"] = img_scaled

        print("")

        # printf("Loaded %d image(s)                                                 \n", len(self.pic_data))

        self.num_rows = len(self.folders)*len(self.orientations)
        # print(self.num_cols)
        # print(self.num_rows)

        self.draw_png()
        QCoreApplication.instance().quit()


    def draw_png(self):

        tw = self.scaled_size.width()*self.num_cols
        th = self.scaled_size.height()*self.num_rows

        image = QImage(tw, th, QImage.Format_ARGB32)

        # painter = QPainter(image)

        painter = QPainter()
        painter.begin(image)

        idx = 0
        for r in range(self.num_rows):
            y = self.scaled_size.height()*r
            for c in range(self.num_cols):
                x = self.scaled_size.width()*c

                img = self.pic_data[self.keys[idx]]["img_scaled"]
                painter.drawImage(x,y,img)


                idx += 1

            


        # # painter.setCompositionMode(QPainter.CompositionMode_Source)

        # x = 0
        # for i in self.krange:
        #     key = self.keys[i]
        #     img = self.pic_data[key]["img_scaled"]
        #     painter.drawImage(x,0,img)
        #     x += img.width()

        painter.end()
        path = self.root+"composite.png"
        image.save(path,"PNG")
        printf("Saved: %s\n", path)



    # junk function
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

