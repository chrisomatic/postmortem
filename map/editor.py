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

class Tile:
    def __init__(self):
        self.tile_index = -1
        self.empty = True
        self.img = None
        self.img_scaled = None

class BoardTile:
    def __init__(self):
        self.tile_index = -1


class Object:
    def __init__(self):
        self.name = ""
        self.img = None
        self.img_scaled = None


class BoardObject():
    def __init__(self):
        self.x = 0
        self.y = 0
        self.obj_index = -1


class Editor(QWidget):
    def __init__(self, parent=None):
        QWidget.__init__(self, parent)

        # number of grid spaces
        self.board_width  = 1000
        self.board_height = 1000

        self.color_clear = QColor(0,0,0,0)
        self.color_rep_clear = QColor(255,0,255,255)

        self.tile_size_actual = 32  # pixels
        self.tile_img_set_width = 512
        self.tile_img_set_height = 512

        self.zoom_ratio = 1

        # scaled
        self.tile_size = int(self.tile_size_actual * self.zoom_ratio)
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

        self.mouse_row, self.mouse_col = self.xy_to_rc(self.mouse_x, self.mouse_y)

        # of the grid
        self.view_tr_x = 0
        self.view_tl_x = 0
        self.view_tl_y = 0
        self.view_bl_y = 0
        self.view_height = 0
        self.view_width = 0

        self.show_grid = True
        self.tool = ""
        self.bsize = 1

        self.tile_set_path = ""
        # self.tile_set_name = ""
        self.tile_set = []

        self.object_set_path = ""
        self.object_set = []

        # 0: terrain, 1: objects
        self.curr_layer = 0

        # tiles or objects
        self.selected_index = 0

        self.board = [[BoardTile() for c in range(self.board_width)] for r in range(self.board_height)]
        # reference --> self.board[row][col]
        self.objects = []

        self.setMouseTracking(True)

    def sub_img_color(self, img, find, replace):
        count = 0
        for y in range(img.height()):
            for x in range(img.width()):
                color = QColor(img.pixel(x,y))
                if(color == find):
                    img.setPixelColor(x,y,replace)
                    count += 1
        return img,count

    def build_objects(self):
        if(not os.path.isdir(self.object_set_path)):
            printf("Folder does not exist: %s\n", self.tile_set_path)
            return

        pngs = [x for x in os.listdir(self.object_set_path) if os.path.isfile(self.object_set_path+x) and x.endswith(".png")]

        if(len(pngs) == 0):
            printf("No objects found")
            return

        self.object_set = []

        for p in pngs:
            print(p)
            path = self.object_set_path + p
            img = QImage(path,"PNG")
            o = Object()
            o.name = p
            o.img,count = self.sub_img_color(img, self.color_rep_clear, self.color_clear)
            o.img_scaled = o.img.scaledToHeight(int(self.zoom_ratio*o.img.height()))
            self.object_set.append(o)

        x = 100
        y = 100
        # w = 0
        # h = 0
        for i in range(len(self.object_set)):
            bo = BoardObject()
            bo.obj_index = i
            bo.x = x
            bo.y = y
            self.objects.append(bo)

            x += 100+self.object_set[i].img_scaled.width()
            y += 100+self.object_set[i].img_scaled.height()




    def build_tiles(self):
        if(not os.path.isfile(self.tile_set_path)):
            printf("File path does not exist: %s\n", self.tile_set_path)
            return

        img = QImage(self.tile_set_path,"PNG")

        if(img.width() != self.tile_img_set_width or img.height() != self.tile_img_set_height):
            printf("Expected %d x %d image size, actual: %d x %d\n", self.tile_img_set_width, self.tile_img_set_height, img.width, img.height)
            return

        # self.tile_set_name = self.tile_set_path.split("\\")[-1]
        self.tile_set = []

        nrow = int(self.tile_img_set_height/self.tile_size_actual)
        ncol = int(self.tile_img_set_width/self.tile_size_actual)
        num_pixels = self.tile_size_actual**2   # per tile

        for i in range(nrow):
            for j in range(ncol):
                t = Tile()
                idx = i*nrow + j
                t.img = img.copy(self.tile_size_actual*j,self.tile_size_actual*i,self.tile_size_actual,self.tile_size_actual)
                t.img,count = self.sub_img_color(t.img, self.color_rep_clear, self.color_clear,)
                t.empty = (count == num_pixels)
                if(t.empty):
                    # printf("index %d is empty\n", idx)
                    pass
                else:
                    t.img_scaled = t.img.scaledToHeight(self.tile_size)
                self.tile_set.append(t)

        # TEST
        row = 0
        col = 0
        for i in range(len(self.tile_set)):
            # if(self.tile_set[i].empty): continue
            self.board[row][col].tile_index = i
            col += 1
            if(col >= 16):
                row += 1
                col = 0

        # for row in range(0,self.board_height):
        #     for col in range(0,self.board_width):
        #         rti = self.rand_tile_index()
        #         self.set_tile(row,col,rti)

    def rand_tile_index(self):
        choices = [x for x in range(len(self.tile_set)) if not(self.tile_set[x].img_scaled is None) and not(self.tile_set[x].empty)]
        if(len(choices) == 0):
            return None
        return choice(choices)

    def set_tile(self, row, col, tile_index):
        if(row >= self.board_height or row < 0):
            return
        if(col >= self.board_width or col < 0):
            return
        if(tile_index >= len(self.tile_set) or tile_index < 0):
            return

        t = self.tile_set[tile_index]
        if(t.img_scaled is None or t.empty):
            return

        self.board[row][col].tile_index = tile_index


    def clear_tile(self, row, col):
        if(row >= self.board_height or row < 0):
            return
        if(col >= self.board_width or col < 0):
            return
        new = BoardTile()
        self.board[row][col] = new

    def update_zoom(self, zoom):
        self.zoom_ratio = zoom
        self.tile_size = int(self.tile_size_actual * self.zoom_ratio)
        self.width = self.board_width*self.tile_size+1
        self.height = self.board_height*self.tile_size+1

        self.setMinimumWidth(self.width+self.tile_size*1)
        self.setMaximumWidth(self.width+self.tile_size*1)
        self.setMinimumHeight(self.height+self.tile_size*1)
        self.setMaximumHeight(self.height+self.tile_size*1)

        self.mouse_row, self.mouse_col = self.xy_to_rc(self.mouse_x, self.mouse_y)

        if(len(self.tile_set) > 0):
            for i in range(len(self.tile_set)):
                t = self.tile_set[i]
                if(not(t.empty) and not(t.img is None)):
                    t.img_scaled = t.img.scaledToHeight(self.tile_size)

        if(len(self.object_set) > 0):
            for i in range(len(self.object_set)):
                o = self.object_set[i]
                if(not(o.img is None)):
                    o.img_scaled = o.img.scaledToHeight(int(self.zoom_ratio*o.img.height()))

    def xy_to_rc(self, x, y):
        col = int(x/self.tile_size)
        row = int(y/self.tile_size)
        return row,col

    def rc_to_xy(self, row, col):
        y = row*self.tile_size
        x = col*self.tile_size
        return x,y


    def paintEvent(self, event):
        # print("paint event")

        painter = QPainter(self)


        self.draw_tiles(painter)
        self.draw_objects(painter)
        self.draw_grid(painter)

        font_size = 14

        # mouse coordinates
        self.draw_text(painter, self.view_tl_x+5, self.view_tl_y+5, self.TOP_LEFT, Qt.black, font_size, "%d,%d", self.mouse_x, self.mouse_y)
        # grid coordinates
        self.draw_text(painter, self.view_tl_x+5, self.view_bl_y-10, self.BOTTOM_LEFT, Qt.black, font_size, "%d,%d", self.mouse_row, self.mouse_col)


    def mouseMoveEvent(self, event):
        self.mouse_x = event.pos().x()
        self.mouse_y = event.pos().y()
        self.mouse_row, self.mouse_col = self.xy_to_rc(self.mouse_x, self.mouse_y)
        self.update()


    def draw_objects(self, painter):
        for bo in self.objects:
            if(bo.obj_index < 0 or bo.obj_index >= len(self.objects)):
                continue

            o = self.object_set[bo.obj_index]
            if(o.img is None):
                continue

            x = int(bo.x*self.zoom_ratio)
            y = int(bo.y*self.zoom_ratio)

            w = o.img_scaled.width()
            h = o.img_scaled.height()

            if((x + w) < self.view_tl_x or x > self.view_tr_x):
                continue

            if((y + h) < self.view_tl_y or y > self.view_bl_y):
                continue

            painter.drawImage(x, y, o.img_scaled)



    def draw_grid(self, painter):
        if(self.tile_size < 4 or not self.show_grid): return
        pen = QPen(Qt.gray, 1, Qt.SolidLine)
        painter.setPen(pen)

        # # draws the entire grid
        # for y in range(0,self.board_height+1):
        #     painter.drawLine(0, int(y*self.tile_size), int(self.width), int(y*self.tile_size))
        # for x in range(0,self.board_width+1):
        #     painter.drawLine(int(x*self.tile_size), 0, int(x*self.tile_size), int(self.height))
        # return

        tl_row, tl_col = self.xy_to_rc(self.view_tl_x, self.view_tl_y)
        br_row, br_col = self.xy_to_rc(self.view_tr_x, self.view_bl_y)


        br_col = min(self.board_width-1, br_col+1)
        br_row = min(self.board_height-1, br_row+1)

        ncols = br_col - tl_col+1
        nrows = br_row - tl_row+1
        w = (ncols)*self.tile_size
        h = (nrows)*self.tile_size

        # print(br_row, tl_row, h, nrows)

        # horizontal lines
        x0,_ = self.rc_to_xy(0,tl_col)
        x1 = int(x0+w)
        for row in range(nrows+1):
            _,y = self.rc_to_xy(tl_row+row,0)
            painter.drawLine(x0, y, x1, y)

        # vertical lines
        _,y0 = self.rc_to_xy(tl_row,0)
        y1 = int(y0+h)
        for col in range(0,ncols+1):
            x,_ = self.rc_to_xy(0,tl_col+col)
            painter.drawLine(x, y0, x, y1)


    def draw_tiles(self, painter):

        # # draw all tiles
        # for row in range(0,self.board_height):
        #     for col in range(0,self.board_width):
        #         ti = self.board[row][col].tile_index
        #         if(ti < 0): continue
        #         t = self.tile_set[ti]
        #         if(t.empty or t.img_scaled is None): continue
        #         x,y = self.rc_to_xy(row,col)
        #         # print(x,y,row,col,ti)
        #         painter.drawImage(x, y, t.img_scaled)

        tl_row, tl_col = self.xy_to_rc(self.view_tl_x, self.view_tl_y)
        br_row, br_col = self.xy_to_rc(self.view_tr_x, self.view_bl_y)

        br_col = min(self.board_width-1, br_col+1)
        br_row = min(self.board_height-1, br_row+1)

        ncols = br_col - tl_col
        nrows = br_row - tl_row
        w = (ncols)*self.tile_size
        h = (nrows)*self.tile_size

        # horizontal lines
        for r in range(0,nrows+1):
            for c in range(0,ncols+1):
                row = r+tl_row
                col = c+tl_col
                ti = self.board[row][col].tile_index
                if(ti < 0): continue
                t = self.tile_set[ti]
                if(t.empty or t.img_scaled is None): continue
                x,y = self.rc_to_xy(row,col)
                # print(x,y,row,col,ti)
                painter.drawImage(x, y, t.img_scaled)


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

        self.first_resize = False
        self.initialized = False
        self.scroll = None
        self.scroll_v_ratio = 0
        self.scroll_h_ratio = 0

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
        font = QFont(self.font_name, 12)
        self.setFont(font)
        QToolTip.setFont(font)

        self.widget = QWidget()
        self.editor = Editor(self)
        self.editor.font_name = self.font_name
        self.editor.setMinimumWidth(self.editor.width+self.editor.tile_size)
        self.editor.setMaximumWidth(self.editor.width+self.editor.tile_size)
        self.editor.setMinimumHeight(self.editor.height+self.editor.tile_size)
        self.editor.setMaximumHeight(self.editor.height+self.editor.tile_size)
        self.scroll = self.scroll_area(self.editor)
        self.scroll.verticalScrollBar().valueChanged.connect(self.scrolled)
        self.scroll.horizontalScrollBar().valueChanged.connect(self.scrolled)


        self.tool_combo = QComboBox(self)
        # self.tool_list = ['Pen','Rectangle','Rectangle Fill','Fill','Copy Range','Objects','Get Zone']
        self.tool_list = ['Pen']
        self.tool_combo.addItems(self.tool_list)
        self.tool_combo.activated[str].connect(self.change_tool)
        self.tool_combo.setToolTip("Ctrl + D")


        self.sld = QSlider(Qt.Horizontal, self)
        self.sld.setMinimum(1)
        self.sld.setMaximum(64)
        # self.sld.setMaximumWidth(600)
        self.sld.valueChanged.connect(self.change_size)
        self.sld.setToolTip("Change the brush size for the pen tool.")

        self.sld_zoom = QSlider(Qt.Horizontal, self)
        self.zoom_values = [1/32,1/16,1/8,.25,.5,1,2,4,8,16]

        self.sld_zoom.setMinimum(1)
        self.sld_zoom.setMaximum(len(self.zoom_values))
        # self.sld_zoom.setMaximumWidth(100)
        self.sld_zoom.valueChanged.connect(self.change_zoom)

        self.save_btn = QPushButton('Save', self)
        self.save_btn.clicked.connect(lambda x: self.save_map(""))
        self.save_btn.resize(self.save_btn.sizeHint())
        self.save_btn.setToolTip("Ctrl + S")

        self.save2_btn = QPushButton('Save PNG', self)
        self.save2_btn.clicked.connect(self.save_png)
        self.save2_btn.resize(self.save2_btn.sizeHint())
        self.save2_btn.setToolTip("Save the map as a png.")

        self.load_btn = QPushButton('Load', self)
        self.load_btn.clicked.connect(lambda x: self.load_map(""))
        self.load_btn.resize(self.load_btn.sizeHint())
        self.load_btn.setToolTip("Ctrl + O")

        self.clear_btn = QPushButton('Clear', self)
        self.clear_btn.clicked.connect(self.clear_map)
        self.clear_btn.resize(self.clear_btn.sizeHint())
        self.clear_btn.setToolTip("Clear the current map.")

        self.undo_btn = QPushButton('Undo', self)
        self.undo_btn.clicked.connect(self.undo)
        self.undo_btn.resize(self.undo_btn.sizeHint())
        self.undo_btn.setToolTip("Ctrl + Z")

        self.draw_over_chk = QCheckBox("Draw Over Tiles",self)
        self.draw_over_chk.setChecked(True)
        self.draw_over_chk.setToolTip("Unselect this to only draw on tiles that are empty.")
        self.draw_over_chk.installEventFilter(self)
        
        self.draw_grid_chk = QCheckBox("Show Grid",self)
        self.draw_grid_chk.setChecked(True)
        self.draw_grid_chk.setToolTip("Unselect this to hide the grid.")
        self.draw_grid_chk.installEventFilter(self)

        self.draw_objs_chk = QCheckBox("Show Objects",self)
        self.draw_objs_chk.setChecked(True)
        self.draw_objs_chk.setToolTip("Unselect this to hide objects.")
        self.draw_objs_chk.installEventFilter(self)

        self.align_grid_chk = QCheckBox("Align To Grid",self)
        self.align_grid_chk.setChecked(True)
        self.align_grid_chk.setToolTip("Select this if you wish to draw objects aligned to the grid.")
        self.align_grid_chk.installEventFilter(self)

        self.lbl_pen_size = QLabel('Pen Size: 1', self)

        self.lbl_zoom = QLabel('Zoom: ', self)


        self.layer_list = ["Terrain","Objects"]
        self.layer_combo = QComboBox(self)
        for i in self.layer_list:
            self.layer_combo.addItem(i)
        self.layer_combo.setCurrentIndex(0)
        self.layer_combo.activated[str].connect(self.show_set_selector)
        self.layer_combo.setToolTip("Ctrl + T")

        self.tile_selector = QListWidget(self)
        self.tile_selector_text = []
        self.tile_selector_grid_row = 0
        self.tile_selector_grid_col = 0
        self.tile_selector_grid_cs = 0
        self.tile_selector_grid_rs = 0

        self.editor.tile_set_path = self.root + "../img/ground_set.png"
        self.editor.build_tiles()
        self.editor.object_set_path = self.root + "../img/objects/"
        self.editor.build_objects()

        self.grid = QGridLayout()
        self.grid.setSpacing(10)

        self.scroll_cs = 30
        self.scroll_rs = 30

        r = 0
        c = 0
        rs = self.scroll_rs
        cs = self.scroll_cs
        self.grid.addWidget(self.scroll, r, c, rs, cs)

        c = cs+c+1
        rs = 1
        cs = 2
        self.grid.addWidget(self.tool_combo, r, c, rs, cs)
        r += 2
        self.grid.addWidget(self.lbl_pen_size, r, c, rs, cs)
        r += 1
        self.grid.addWidget(self.sld, r, c, rs, cs)

        r += 2
        self.grid.addWidget(self.lbl_zoom, r, c, rs, cs)
        r += 1
        self.grid.addWidget(self.sld_zoom, r, c, rs, cs)


        r += 2
        self.grid.addWidget(self.draw_over_chk, r, c, rs, 1)
        r += 1
        self.grid.addWidget(self.draw_grid_chk, r, c, rs, 1)
        r += 1
        self.grid.addWidget(self.draw_objs_chk, r, c, rs, 1)
        r += 1
        self.grid.addWidget(self.align_grid_chk, r, c, rs, 1)

        r += 2
        self.grid.addWidget(self.layer_combo, r, c, rs, 2)

        r += 1
        self.tile_selector_grid_row = r
        self.tile_selector_grid_col = c
        self.tile_selector_grid_cs = 2
        self.tile_selector_grid_rs = 10
        self.grid.addWidget(self.tile_selector, r, c, self.tile_selector_grid_rs, self.tile_selector_grid_cs)

        r = 0
        c = cs+c+1
        self.grid.addWidget(self.save_btn, r, c, rs, cs)
        r += 1
        self.grid.addWidget(self.load_btn, r, c, rs, cs)
        r += 1
        self.grid.addWidget(self.save2_btn, r, c, rs, cs)

        r += 2
        self.grid.addWidget(self.clear_btn, r, c, rs, cs)
        r += 1
        self.grid.addWidget(self.undo_btn, r, c, rs, cs)


        self.widget.setLayout(self.grid)
        self.setCentralWidget(self.widget)


        self.show_set_selector("")

        self.sld_zoom.setValue(self.zoom_values.index(1)+1)

        self.initialized = True

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



        # print(self.editor.vkiew_tl_y)

        # v = self.scroll.verticalScrollBar().value()
        # m = self.scroll.verticalScrollBar().maximum()
        # if(m <= 0):
        #     self.scroll_v_ratio = 0
        # else:
        #     self.scroll_v_ratio = v/m
        # # print("self.scroll_v_ratio", self.scroll_v_ratio)

        # v = self.scroll.horizontalScrollBar().value()
        # m = self.scroll.horizontalScrollBar().maximum()
        # if(m <= 0):
        #     self.scroll_h_ratio = 0
        # else:
        #     self.scroll_h_ratio = v/m

    def scrolled(self):
        self.repaint()


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
        if(not(self.first_resize) and self.width() != 200 and self.height() != 200):
            self.first_resize = True
            self.w = self.width()
            self.h = self.height()
            print("resize",self.w,self.h)
            rect = self.frameGeometry()
            self.setGeometry(rect)
            self.showNormal()
            self.init()
        else:
            self.update_scroll_info()


    def eventFilter(self, source, event):
        if(event is None): return 0
        # if(event.type() == QEvent.Scroll):
        #     print("scroll")

        if(not(self.initialized)): return 0

        if source is self.draw_grid_chk:
            self.editor.show_grid = self.draw_grid_chk.isChecked()
            self.repaint()

        elif source is self.scroll:
            print("scroll")

        # elif source is self.draw_over_chk:
        #     self.editor.draw_over = self.draw_over_chk.isChecked()

        # elif source is self.draw_objs_chk:
        #     self.editor.draw_objs = self.draw_objs_chk.isChecked()
        #     self.editor.update()

        # elif source is self.align_grid_chk:
        #     self.editor.align_objs = self.align_grid_chk.isChecked()






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


    def change_tool(self,text):
        self.editor.tool = text.lower()


    def change_size(self):
        self.editor.bsize = max(1,int(self.sld.value()))
        self.lbl_pen_size.setText("Pen Size: " + str(self.editor.bsize))

    def change_size(self):
        self.editor.bsize = max(1,int(self.sld.value()))
        self.lbl_pen_size.setText("Pen Size: " + str(self.editor.bsize))


    def save_png(self):
        return

    def undo(self):
        return

    def clear_map(self):
        return

    def save_map(self,fname = ""):
        return

    def load_map(self,fname=""):
        return

    def create_eraser_item(self):
        self.eraser_item = QListWidgetItem()
        self.eraser_item.setText("Eraser")
        if os.path.isfile("eraser.png"):
            self.eraser_item.setIcon(QIcon("eraser.png"))

    def show_set_selector(self,text):
        sel = self.layer_combo.currentText()

        items = []
        text = []

        self.tile_selector.clear()
        self.tile_selector_text = []
        self.tile_selector_indexes = []
        self.editor.selected_index = -1
        self.create_eraser_item()
        self.tile_selector.addItem(self.eraser_item)

        if(sel == "Terrain"):
            self.editor.curr_layer = 0
            if(len(self.editor.tile_set) > 0):
                for i in range(len(self.editor.tile_set)):
                    t = self.editor.tile_set[i]
                    if(t.img is None or t.empty): continue
                    item = QListWidgetItem()
                    _str = 'Tile ' + str(i)
                    item.setText(_str)
                    item.setIcon(QIcon(QPixmap.fromImage(t.img)))
                    items.append(item)
                    text.append(_str)
                    self.tile_selector_indexes.append(i)

        elif(sel == "Objects"):
            self.editor.curr_layer = 1
            if(len(self.editor.object_set) > 0):
                for i in range(len(self.editor.object_set)):
                    o = self.editor.object_set[i]
                    item = QListWidgetItem()
                    _str = o.name
                    item.setText(_str)
                    item.setIcon(QIcon(QPixmap.fromImage(o.img)))
                    items.append(item)
                    text.append(_str)
                    self.tile_selector_indexes.append(i)


        for i in range(len(items)):
            self.tile_selector.addItem(items[i])
            self.tile_selector_text.append(text[i])

        self.tile_selector.itemClicked.connect(self.tile_selector_clicked)

        self.grid.addWidget(self.tile_selector, self.tile_selector_grid_row, self.tile_selector_grid_col, self.tile_selector_grid_rs, self.tile_selector_grid_cs)
        self.widget.setLayout(self.grid)
        self.setCentralWidget(self.widget)

    def tile_selector_clicked(self,curr):
    
        self.tile_selector.selectedItems()
        itext = curr.text()
        idx = -1

        if(itext == "Eraser"):
            idx = -1
        else:
            if(self.editor.curr_layer == 0):
                idx = self.tile_selector_text.index(itext)
            else:
                idx = self.tile_selector_text.index(itext)


        if(idx < 0):
            self.editor.selected_index = idx
        else:
            self.editor.selected_index = self.tile_selector_indexes[idx]

        # print(itext, "|", self.editor.selected_index)


    def change_zoom(self):
        value = self.sld_zoom.value()
        zoom = 1/self.zoom_values[value-1]
        if zoom == int(zoom):
            l = str(int(zoom))
        elif zoom == 1/2:
            l = "1/2"
        elif zoom == 1/4:
            l = "1/4"
        elif zoom == 1/8:
            l = "1/8"
        elif zoom == 1/16:
            l = "1/16"
        elif zoom == 1/32:
            l = "1/32"
        self.lbl_zoom.setText("Zoom: x" + l)


        # self.scroll = self.scroll_area(self.editor)

        # # v = int(self.scroll_v_ratio*self.scroll.verticalScrollBar().maximum())
        # # print("setting v", v)
        # # print("height", self.scroll.viewport().height())


        # self.scroll.verticalScrollBar().valueChanged.connect(self.scrolled)
        # self.scroll.horizontalScrollBar().valueChanged.connect(self.scrolled)


        # v = self.editor.view_tl_y*self.editor.zoom_ratio
        # self.scroll.verticalScrollBar().setValue(int(v))
        # print("new value:", v)

        # self.grid.addWidget(self.scroll, 0, 0, self.scroll_rs, self.scroll_rs)
        # self.widget.setLayout(self.grid)
        # self.setCentralWidget(self.widget)

        self.editor.update_zoom(zoom)
        # self.editor.update()
        self.repaint()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    w = MainWindow()
    w.show()
    # app.setQuitOnLastWindowClosed(False)
    app.exec_()

