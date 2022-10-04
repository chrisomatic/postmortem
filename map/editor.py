from time import time, sleep
from random import choice
import sys, os
import math
from PyQt5.QtWidgets import *
from PyQt5.QtGui import QKeyEvent, QPainter,QImage, QPen, QIcon, QPixmap, QColor, QBrush, QCursor, QFont, QPalette, QTransform, QLinearGradient, QFontMetrics, QStaticText
from PyQt5.QtCore import Qt, QPoint, QPointF, QSize, QEvent, QTimer, QCoreApplication, QRect
from datetime import datetime

def bound(_val, _min, _max):
    return max(min(_val, _max), _min)

def printf(fmt, *args):
    if(len(args) > 0):
        print(fmt % args, end="")
    else:
        print(fmt, end="")

class Editor(QWidget):
    def __init__(self, parent=None):
        QWidget.__init__(self, parent)

        self.board_width  = 1000
        self.board_height = 1000
        self.tile_size = 32

        self.width = self.board_width*self.tile_size+1
        self.height = self.board_height*self.tile_size+1

        self.font_name = ""

        # text orientation
        self.NONE = -1
        self.CENTERED = 0
        self.TOP_LEFT = 1
        self.TOP_RIGHT = 2
        self.BOTTOM_LEFT = 3
        self.BOTTOM_RIGHT = 4
        self.CENTERED_TOP = 5
        self.CENTERED_BOTTOM = 6
        self.CENTERED_LEFT = 7

        gpos = QCursor.pos()
        pos = self.mapFromGlobal(gpos)
        self.mouse_x = pos.x()
        self.mouse_y = pos.y()

        self.grid_x = 0
        self.grid_y = 0
        self.grid_x, self.grid_y = self.get_grid_coords(self.mouse_x, self.mouse_y)

        # of the grid
        self.view_tr_x = 0
        self.view_tl_x = 0
        self.view_tl_y = 0
        self.view_bl_y = 0
        self.view_height = 0
        self.view_width = 0

        self.setMouseTracking(True)

    def get_grid_coords(self,x,y):
        grid_x = int(x/self.tile_size)
        grid_y = int(y/self.tile_size)
        return grid_x, grid_y

    def paintEvent(self, event):

        painter = QPainter(self)
        # self.draw_tiles()
        # if self.draw_objs:
        #     self.draw_objects()

        font_size = 14

        # grid
        self.draw_grid(painter)


        # mouse coordinates
        self.draw_text(painter, self.view_tl_x+5, self.view_tl_y+5, self.TOP_LEFT, Qt.black, font_size, "%d,%d", self.mouse_x, self.mouse_y)
        # grid coordinates
        self.draw_text(painter, self.view_tl_x+5, self.view_bl_y-10, self.BOTTOM_LEFT, Qt.black, font_size, "%d,%d", self.grid_x, self.grid_y)


        # self.draw_rect_wh(self.h_lbound,self.v_lbound)
        # self.draw_coords(self.h_lbound+2,self.v_ubound-5,int(int(self.mouse_x) * self.tile_size),int(int(self.mouse_y) * self.tile_size))
        # self.draw_coords(self.h_ubound-55,self.v_ubound-5,int(self.mouse_x), int(self.mouse_y))
        
        # self.draw_copy_status(self.h_lbound + (self.h_ubound - self.h_lbound)*.45,self.v_ubound-5)
        # self.draw_get_zone(self.h_lbound + (self.h_ubound - self.h_lbound)*.45,self.v_ubound-5)
        # self.drawGhostRect()

    def mouseMoveEvent(self, event):
        self.mouse_x = event.pos().x()
        self.mouse_y = event.pos().y()
        self.grid_x, self.grid_y = self.get_grid_coords(self.mouse_x, self.mouse_y)
        self.update()


    def draw_grid(self, painter):
            pen = QPen(Qt.gray, 1, Qt.SolidLine)
            painter.setPen(pen)

            for y in range(0,self.board_height+1):
                painter.drawLine(0, int(y*self.tile_size), int(self.width), int(y*self.tile_size))

            for x in range(0,self.board_width+1):
                painter.drawLine(int(x*self.tile_size), 0, int(x*self.tile_size), int(self.height))



            # for y in range(0,self.board_height):
            #     qp.drawLine(int(y), 0, int(y), int(4096))
            # for x in range(0,self.board_width):
            #     qp.drawLine(0, int(x*self.tile_size_zoom), int(4096*self.zoom_ratio), int(x*self.tile_size_zoom))
            # qp.end()


    def draw_text(self, painter, x, y, orientation, color, size, fmt, *args):

        if(len(args) > 0):
            _str = fmt % args
        else:
            _str = fmt

        if(len(_str) == 0):
            return 0,0,None,None,None


        pen = QPen()
        pen.setWidth(1)
        pen.setColor(color)
        painter.setPen(pen)
        painter.setBrush(color)
        font = QFont(self.font_name)
        font.setPixelSize(size)
        painter.setFont(font)
        fm = QFontMetrics(font)

        # w = fm.width(_str)
        # h = fm.height()
        # a = fm.ascent()
        # d = fm.descent()
        # l = fm.leading()
        lb = fm.leftBearing(_str[0])

        x_adj = x
        y_adj = y

        rect = fm.tightBoundingRect(_str)
        w = rect.topRight().x() - rect.topLeft().x()
        h = rect.bottomLeft().y() - rect.topLeft().y()

        if(orientation == self.NONE):
            x_adj = x
            y_adj = y
        elif(orientation == self.CENTERED):
            x_adj = x-w/2-lb/2
            y_adj = y+h/2
        elif(orientation == self.TOP_LEFT):
            x_adj = x
            y_adj = y+h
        elif(orientation == self.TOP_RIGHT):
            x_adj = x-w
            y_adj = y+h
        elif(orientation == self.BOTTOM_LEFT):
            x_adj = x
            y_adj = y
        elif(orientation == self.BOTTOM_RIGHT):
            x_adj = x-w
            y_adj = y
        elif(orientation == self.CENTERED_TOP):
            x_adj = x-w/2-lb/2
            y_adj = y+h
        elif(orientation == self.CENTERED_BOTTOM):
            x_adj = x-w/2-lb/2
            y_adj = y
        elif(orientation == self.CENTERED_LEFT):
            x_adj = x
            y_adj = y+h/2

        xdelta = x_adj-x
        ydelta = y_adj-y

        painter.drawText(int(x_adj), int(y_adj), _str)
        # painter.drawStaticText(QPoint(x,y),QStaticText(_str))
        # self.draw_circle(painter, int(x), int(y), 1, 1, self.color_none, Qt.blue)
        # self.draw_circle(painter, int(x_adj), int(y_adj), 1, 1, self.color_none, Qt.red)
        return w,h,fm,xdelta,ydelta

class MainWindow(QMainWindow):
    def __init__(self, parent=None):
        print("__init__()")
        QWidget.__init__(self, parent)

        self.initialized = False
        self.scroll = None


        self.r = 128
        self.g = self.r
        self.b = self.r
        self.setStyleSheet("background-color: rgba(%d, %d, %d, 128);" % (self.r, self.g, self.b))

        # self.aspect_ratio = 16/9
        self.desktop = QDesktopWidget()
        self.screen_count = self.desktop.screenCount()
        self.screen_index = min(1,self.screen_count-1)
        self.screen = self.desktop.screenGeometry(self.screen_index) # select monitor if available
        self.desktop.activateWindow()
        self.screen_w = self.screen.width()
        self.screen_h = self.screen.height()

        self.w = 200
        self.h = 200
        # self.setGeometry(int((self.screen_w-self.w)/2), int((self.screen_h-self.h)/2), self.w, self.h)
        self.setGeometry(0, 0, self.w, self.h)

        self.show()
        self.setWindowState(Qt.WindowMaximized)

    def init(self):
        print("init()")

        self.setWindowTitle("Map Editor")
        self.installEventFilter(self)
        # self.setAcceptDrops(True)

        self.root = os.path.dirname(os.path.abspath(__file__))  + "/"
        
        self.font_name = "Courier"
        font = QFont(self.font_name, 9)
        self.setFont(font)
        QToolTip.setFont(font)

        self.widget = QWidget()
        self.editor = Editor(self)
        self.editor.font_name = self.font_name
        self.editor.setMinimumWidth(self.editor.width)
        self.editor.setMaximumWidth(self.editor.width)
        self.editor.setMinimumHeight(self.editor.height)
        self.editor.setMaximumHeight(self.editor.height)
        self.scroll = self.scroll_area(self.editor)
        self.scroll.verticalScrollBar().valueChanged.connect(self.repaint)
        self.scroll.horizontalScrollBar().valueChanged.connect(self.repaint)

        self.grid = QGridLayout()
        self.grid.setSpacing(10)
        self.grid.addWidget(self.scroll, 0, 0, 10, 10)
        self.widget.setLayout(self.grid)
        self.setCentralWidget(self.widget)

        # print("before repaint")
        self.repaint()


    def update_scroll_info(self):
        if(self.scroll is None): return
        # print(self.scroll.viewport().height())
        self.editor.view_height = self.scroll.viewport().height()
        self.editor.view_width = self.scroll.viewport().width()
        self.editor.view_tl_y = self.scroll.verticalScrollBar().value()
        self.editor.view_bl_y = self.editor.view_tl_y+self.editor.view_height
        self.editor.view_tl_x = self.scroll.horizontalScrollBar().value()
        self.editor.view_tr_x = self.editor.view_tl_x + self.editor.view_width

    def repaint(self):
        # print("repaint")
        self.update_scroll_info()
        self.editor.update()

    def scroll_area(self,_obj):
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setVerticalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded) 
        scroll.setWidget(_obj)
        return scroll


    def resizeEvent(self, event):
        if(not(self.initialized) and self.width() != 200 and self.height() != 200):
            self.initialized = True
            self.w = self.width()
            self.h = self.height()
            rect = self.frameGeometry()
            self.setGeometry(rect)
            self.showNormal()
            self.init()
        else:
            self.update_scroll_info()


    def eventFilter(self, source, event):
        if(event is None): return 0
        if(event.type() == QEvent.KeyPress):
            key = event.key()
            modifiers = QApplication.keyboardModifiers()
            if(modifiers == Qt.ControlModifier):
                if(key == Qt.Key_C):
                    self.custom_close()
            elif(modifiers == Qt.NoModifier):
                if(key == Qt.Key_Q):
                    self.custom_close()
        return 0

    def custom_close(self):
        QCoreApplication.instance().quit()

    def closeEvent(self, event):
        self.custom_close()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    w = MainWindow()
    w.show()
    # app.setQuitOnLastWindowClosed(False)
    app.exec_()

