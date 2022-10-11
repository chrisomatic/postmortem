from time import time, sleep
from random import choice
from datetime import datetime
import sys, os
import math
import struct
from PyQt5.QtWidgets import *
from PyQt5.QtGui import QKeyEvent, QPainter,QImage, QPen, QIcon, QPixmap, QColor, QBrush, QCursor, QFont, QPalette, QTransform, QLinearGradient, QFontMetrics, QStaticText
from PyQt5.QtCore import Qt, QPoint, QPointF, QSize, QEvent, QTimer, QCoreApplication, QRect
import copy

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
        self.view_c1_x = 0
        self.view_c1_y = 0
        self.view_c4_x = 0
        self.view_c4_y = 0
        self.view_height = 0
        self.view_width = 0

        self.tile_set_name = "" #file name
        self.tile_set_path = ""
        self.tile_set = []

        self.object_set_path = ""
        self.object_set = []
        self.object_opacity = 100

        self.show_grid = True
        self.show_objects = True
        self.show_terrain = True
        self.align_objects = True
        self.draw_over = True


        self.rect_c1_r = 0
        self.rect_c1_c = 0
        self.rect_c4_r = 0
        self.rect_c4_c = 0

        self.rectangling = False
        self.drawing = False
        self.tool = "pen"
        self.pen_size = 1

        # 0: terrain, 1: objects
        self.curr_layer = 0

        # tiles or objects
        self.selected_index = 0

        # reference --> self.board[row][col]
        self.board = [[BoardTile() for c in range(self.board_width)] for r in range(self.board_height)]

        self.change_list = []
        self.change_list_disabled = False

        self.objects = []

        self.right_click_cb = None

        self.save_png = False
        self.png_path = ""

        self.editor_undo()

        self.setMouseTracking(True)

    def change_list_add(self, r, c, ti):
        # if(self.tool == "fill"): return
        if(self.change_list_disabled): return
        if(r >= self.board_height): return
        if(c >= self.board_width): return
        for i in range(len(self.change_list)):
            _r,_c,_ti = self.change_list[i]
            if(_r == r and _c == c):
                return
        self.change_list.append((r,c,ti))


    def editor_undo(self):
        for i in range(len(self.change_list)):
            r,c,ti = self.change_list[i]
            ti_prev = self.board[r][c].tile_index
            self.change_list[i] = (r,c,ti_prev)
            self.board[r][c].tile_index = ti


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
            printf("Folder does not exist: %s\n", self.object_set_path)
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

        # x = 100
        # y = 100
        # # w = 0
        # # h = 0
        # for i in range(len(self.object_set)):
        #     bo = BoardObject()
        #     bo.obj_index = i
        #     bo.x = x
        #     bo.y = y
        #     self.objects.append(bo)
        #     x += 100+self.object_set[i].img_scaled.width()
        #     y += 100+self.object_set[i].img_scaled.height()


    def build_tiles(self):
        fpath = self.tile_set_path + self.tile_set_name
        if(not os.path.isfile(fpath)):
            printf("File path does not exist: %s\n", fpath)
            return

        img = QImage(fpath,"PNG")

        if(img.width() != self.tile_img_set_width or img.height() != self.tile_img_set_height):
            printf("Expected %d x %d image size, actual: %d x %d\n", self.tile_img_set_width, self.tile_img_set_height, img.width, img.height)
            return

        # self.tile_set_name = self.tile_set_path.split(slash)[-1]
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


    def rand_tile_index(self):
        choices = [x for x in range(len(self.tile_set)) if not(self.tile_set[x].img_scaled is None) and not(self.tile_set[x].empty)]
        if(len(choices) == 0):
            return None
        return choice(choices)

    def set_tile_fast(self, row, col, tile_index):
        ti_prev = self.board[row][col].tile_index
        self.board[row][col].tile_index = tile_index
        self.change_list_add(row,col,ti_prev)

    def set_tile(self, row, col, tile_index, force):
        if(row >= self.board_height or row < 0):
            return
        if(col >= self.board_width or col < 0):
            return
        if(tile_index >= len(self.tile_set)):
            return

        t = self.tile_set[tile_index]
        if(t.img_scaled is None or t.empty):
            return

        # erase
        if(tile_index < 0):
            self.clear_tile(row, col)
            return

        if(self.board[row][col].tile_index != -1 and not(self.draw_over) and not(force)):
            return

        ti_prev = self.board[row][col].tile_index+0
        self.board[row][col].tile_index = tile_index
        # printf("set tile: %d,%d  %d -> %d\n", row,col,ti_prev,tile_index)
        self.change_list_add(row,col,ti_prev)


    def clear_tile(self, row, col):
        if(row >= self.board_height or row < 0):
            return
        if(col >= self.board_width or col < 0):
            return
        ti_prev = self.board[row][col].tile_index+0
        new = BoardTile()
        self.board[row][col] = new
        self.change_list_add(row,col,ti_prev)


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


    def draw_png(self):
        if(not(self.save_png)): return

        # size = self.editor.tile_size_actual
        size = 2

        tile_set = []
        for i in range(len(self.tile_set)):
            #  = self.editor.tile_set[i].scaledToHeight(int(self.zoom_ratio*o.img.height()))
            img = self.tile_set[i].img.scaledToHeight(size)
            # print(img.width(), img.height())
            tile_set.append(img)

        rows = self.board_height
        cols = self.board_width

        width = cols * size
        height = rows * size
        # print(width,height)
        image = QImage(width, height, QImage.Format_RGB32)

        painter = QPainter(image)
        # painter.begin(image)

        for r in range(rows):
            for c in range(cols):
                x = c*size
                y = r*size
                ti = self.board[r][c].tile_index
                if(ti == 0xFF or ti == -1): continue
                painter.drawImage(x,y,tile_set[ti])

        painter.end()
        image.save(self.png_path)

        self.png_path = ""
        self.save_png = False

    def draw_ghost(self, painter):

        x,y,x2,y2 = self.get_tool_area()
        w = abs(x2-x)
        h = abs(y2-y)
        # row span, col span
        rs = int(w/self.tile_size)
        cs = int(h/self.tile_size)

        brush = QBrush(QColor(128, 128, 255, 128))

        erasing_objects = (self.curr_layer == 1 and self.selected_index == -1)

        if(self.tool == "fill" and erasing_objects):

            x,y,x2,y2 = self.get_pen_area(self.align_objects)
            w = abs(x2-x)
            h = abs(y2-y)
            painter.fillRect(x,y,w,h,brush)

        elif(self.tool == "rectangle" and not(erasing_objects or cs == 1 or rs == 1)):
            # top
            painter.fillRect(x,y,w,self.tile_size,brush)
            # bottom
            painter.fillRect(x,y+h-self.tile_size,w,self.tile_size,brush)

            # left
            painter.fillRect(x,y+self.tile_size,self.tile_size,h-self.tile_size*2,brush)
            # right
            painter.fillRect(x+w-self.tile_size,y+self.tile_size,self.tile_size,h-self.tile_size*2,brush)

        else:
            painter.fillRect(x,y,w,h,brush)





    def paintEvent(self, event):
        # print("paint event")


        self.draw_png()

        painter = QPainter(self)
        self.draw_terrain(painter)
        self.draw_objects(painter)
        self.draw_ghost(painter)
        self.draw_grid(painter)

        font_size = 14

        # mouse coordinates
        self.draw_text(painter, self.view_c1_x+5, self.view_c1_y+5, self.TOP_LEFT, Qt.black, font_size, "%d,%d", self.mouse_x, self.mouse_y)
        # grid coordinates
        self.draw_text(painter, self.view_c1_x+5, self.view_c4_y-10, self.BOTTOM_LEFT, Qt.black, font_size, "%d,%d", self.mouse_row, self.mouse_col)



    def check_object_erase(self, x1, y1, x2, y2):

        ex = min(x1,x2)
        ey = min(y1,y2)
        ew = abs(x1-x2)
        eh = abs(y1-y2)

        new_lst = []
        for i in range(len(self.objects)):
            ob = self.objects[i]
            o = self.object_set[ob.obj_index]
            ow = o.img_scaled.width()
            oh = o.img_scaled.height()
            ox = ob.x*self.zoom_ratio
            oy = ob.y*self.zoom_ratio

            collide = ((ox + ow) > ex and (oy + oh) > ey and (ex + ew) > ox and (ey + eh) > oy)
            if(not(collide)):
                new_lst.append(ob)

        self.objects = new_lst



    def object_tool(self, action):

        if(self.selected_index >= len(self.object_set)):
            return

        x1,y1,x2,y2 = self.get_tool_area()


        if(action == "press"):
            self.drawing = True

            self.drawing = True
            ob = BoardObject()
            ob.obj_index = self.selected_index
            ob.x = int(x1/self.zoom_ratio)
            ob.y = int(y1/self.zoom_ratio)
            self.objects.append(ob)

        elif(action == "move"):
            self.drawing = False

        elif(action == "release"):
            self.drawing = False


    def mouse_handle_tool(self, action):

        erasing_objects = (self.curr_layer == 1 and self.selected_index == -1)

        # drawing tiles, or erasing objects
        if(self.curr_layer == 0 or erasing_objects):

            if(self.tool == "pen"):
                self.pen_tool(action)

            elif(self.tool in ["rectangle","rectangle fill"]):
                self.rectangle_tool(action)

            elif(self.tool == "fill"):
                self.fill_tool(action)

        else:
            self.object_tool(action)


    def get_tool_area(self):
        erasing_objects = (self.curr_layer == 1 and self.selected_index == -1)

        align = True
        if(self.curr_layer == 1 and not(self.align_objects)):
            align = False

        if(self.curr_layer == 1 and self.selected_index >= 0):
            return self.get_object_area(align)

        elif(self.curr_layer == 1 and erasing_objects):
            return self.get_pen_area(self.align_objects)

        else:
            if(self.tool == "pen"):
                return self.get_pen_area(align)

            elif(self.tool in ["rectangle","rectangle fill"]):
                return self.get_rectangle_area()

            elif(self.tool == "fill"):
                return self.get_fill_area()

        return (0,0,0,0)

    def get_fill_area(self):
        x1 = self.mouse_col*self.tile_size
        y1 = self.mouse_row*self.tile_size
        x2 = x1+self.tile_size
        y2 = y1+self.tile_size
        return (x1,y1,x2,y2)

    def get_object_area(self, aligned):

        if(aligned):
            x1 = self.mouse_col*self.tile_size
            y1 = self.mouse_row*self.tile_size
        else:
            x1 = self.mouse_x
            y1 = self.mouse_y

        o = self.object_set[self.selected_index]
        w = o.img_scaled.width()
        h = o.img_scaled.height()

        # area of object image
        return (x1,y1,x1+w,y1+h)

    def get_pen_area(self, aligned):

        if(aligned):
            x = self.mouse_col*self.tile_size
            y = self.mouse_row*self.tile_size
        else:
            x = self.mouse_x
            y = self.mouse_y

        side = int((self.pen_size-1)/2)*self.tile_size
        rem = ((self.pen_size-1) % 2)*self.tile_size
        x1 = x-side
        x2 = x+side+rem+self.tile_size
        y1 = y-side
        y2 = y+side+rem+self.tile_size
        return (x1,y1,x2,y2)

    def get_rectangle_area(self):
        r1 = self.rect_c1_r
        c1 = self.rect_c1_c
        r4 = self.rect_c4_r
        c4 = self.rect_c4_c
        if(r4 >= r1):
            r4 += 1
        else:
            r1 += 1

        if(c4 >= c1):
            c4 += 1
        else:
            c1 += 1

        cs = abs(c4-c1)
        rs = abs(r4-r1)

        x1 = min(c1,c4)*self.tile_size
        y1 = min(r1,r4)*self.tile_size
        w = self.tile_size*cs
        h = self.tile_size*rs
        x2 = x1+w
        y2 = y1+h

        return x1,y1,x2,y2

    # also erases tiles
    def draw_tiles(self, row1, col1, row2, col2, filled):
        rows = range(min([row1,row2]),max([row1,row2]))
        cols = range(min([col1,col2]),max([col1,col2]))
        for r in rows:
            for c in cols:
                if(filled):
                    self.set_tile(r,c, self.selected_index, False)
                else:
                    if(r in [rows[0],rows[-1]] or c in [cols[0],cols[-1]]):
                        self.set_tile(r,c, self.selected_index, False)


    def fill_tool(self, action):
        erasing_objects = (self.curr_layer == 1 and self.selected_index == -1)

        if(erasing_objects):
            self.pen_tool(action)

        x1,y1,x2,y2 = self.get_tool_area()
        r1,c1 = self.xy_to_rc(x1,y1)
        # r2,c2 = self.xy_to_rc(x2,y2)

        if(action == "press"):

            target_tile_index = self.board[r1][c1].tile_index

            if(target_tile_index == self.selected_index):
                return

            self.set_tile_fast(r1,c1,self.selected_index)

            node = (r1,c1)
            queue = []
            queue.append(node)

            while(queue != []):
                n = queue[0]
                queue = queue[1:]

                r = n[0]
                c = n[1]


                # right
                if(c+1 < self.board_width):
                    if(self.board[r][c+1].tile_index == target_tile_index):
                        self.set_tile_fast(r,c+1,self.selected_index)
                        queue.append((r,c+1))

                # left
                if(c-1 >= 0):
                    if(self.board[r][c-1].tile_index == target_tile_index):
                        self.set_tile_fast(r,c-1,self.selected_index)
                        queue.append((r,c-1))

                # up
                if(r-1 >= 0):
                    if(self.board[r-1][c].tile_index == target_tile_index):
                        self.set_tile_fast(r-1,c,self.selected_index)
                        queue.append((r-1,c))

                # down
                if(r+1 < self.board_height):
                    if(self.board[r+1][c].tile_index == target_tile_index):
                        self.set_tile_fast(r+1,c,self.selected_index)
                        queue.append((r+1,c))


    def pen_tool(self, action):

        erasing_objects = (self.curr_layer == 1 and self.selected_index == -1)

        x1,y1,x2,y2 = self.get_tool_area()
        r1,c1 = self.xy_to_rc(x1,y1)
        r2,c2 = self.xy_to_rc(x2,y2)

        if(action == "press"):
            # print("drawing")
            self.drawing = True
            if(erasing_objects):
                self.check_object_erase(x1,y1,x2,y2)
            else:
                self.draw_tiles(r1, c1, r2, c2, True)

        elif(action == "move"):
            # print("moving")
            if(self.drawing):

                if(erasing_objects):
                    self.check_object_erase(x1,y1,x2,y2)
                else:
                    self.draw_tiles(r1, c1, r2, c2, True)

        elif(action == "release"):
            # print("released")
            self.drawing = False

    def rectangle_tool(self, action):

        erasing_objects = (self.curr_layer == 1 and self.selected_index == -1)

        if(action == "press"):
            self.rectangling = True
            self.rect_c1_r = self.mouse_row
            self.rect_c1_c = self.mouse_col
            self.rect_c4_r = self.rect_c1_r
            self.rect_c4_c = self.rect_c1_c

        elif(action == "move"):
            if(self.rectangling):
                self.rect_c4_r = self.mouse_row
                self.rect_c4_c = self.mouse_col
            else:
                self.rect_c1_r = self.mouse_row
                self.rect_c1_c = self.mouse_col
                self.rect_c4_r = self.rect_c1_r
                self.rect_c4_c = self.rect_c1_c

        elif(action == "release"):
            if(self.rectangling):

                self.rect_c4_r = self.mouse_row
                self.rect_c4_c = self.mouse_col

                x1,y1,x2,y2 = self.get_rectangle_area()
                r1,c1 = self.xy_to_rc(x1,y1)
                r2,c2 = self.xy_to_rc(x2,y2)

                filled = (self.tool == "rectangle fill")
                if(erasing_objects):
                    self.check_object_erase(x1,y1,x2,y2)
                else:
                    self.draw_tiles(r1, c1, r2, c2, filled)

            self.rectangling = False
            self.rect_c1_r = self.mouse_row
            self.rect_c1_c = self.mouse_col
            self.rect_c4_r = self.rect_c1_r
            self.rect_c4_c = self.rect_c1_c

    def mousePressEvent(self, event):

        if(event.button() == Qt.LeftButton):
            # print("press")

            self.change_list = []

            # self.board_prev = self.board.copy()
            # self.copy_board_to_prev()

            self.mouse_handle_tool("press")

        elif(event.button() ==  Qt.RightButton):
            if(self.curr_layer == 0):
                self.mouse_x = event.pos().x()
                self.mouse_y = event.pos().y()
                self.mouse_row, self.mouse_col = self.xy_to_rc(self.mouse_x, self.mouse_y)
                if(self.mouse_row < self.board_height and self.mouse_col < self.board_width):
                    ti = self.board[self.mouse_row][self.mouse_col].tile_index
                    if(not(self.right_click_cb is None)):
                        self.right_click_cb(ti)


        self.update()

    def mouseMoveEvent(self, event):
        if(not(event is None)):
            self.mouse_x = event.pos().x()
            self.mouse_y = event.pos().y()
            self.mouse_row, self.mouse_col = self.xy_to_rc(self.mouse_x, self.mouse_y)

        self.mouse_handle_tool("move")

        self.update()


    def mouseReleaseEvent(self, event):

        if(event.button() == Qt.LeftButton):

            self.mouse_handle_tool("release")

        self.update()

    def draw_objects(self, painter):
        if(not(self.show_objects)): return
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

            if((x + w) < self.view_c1_x or x > self.view_c4_x):
                continue

            if((y + h) < self.view_c1_y or y > self.view_c4_y):
                continue

            painter.setOpacity(self.object_opacity)

            painter.drawImage(x, y, o.img_scaled)

        painter.setOpacity(1)



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

        tl_row, tl_col = self.xy_to_rc(self.view_c1_x, self.view_c1_y)
        br_row, br_col = self.xy_to_rc(self.view_c4_x, self.view_c4_y)


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


    def draw_terrain(self, painter):
        if(not(self.show_terrain)): return

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

        tl_row, tl_col = self.xy_to_rc(self.view_c1_x, self.view_c1_y)
        br_row, br_col = self.xy_to_rc(self.view_c4_x, self.view_c4_y)

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

        self.root = os.path.dirname(os.path.abspath(__file__))  + slash

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
        self.tool_list = ["Pen","Rectangle Fill","Rectangle","Fill"]
        self.tool_combo.addItems(self.tool_list)
        self.tool_combo.activated[str].connect(self.change_tool)
        self.tool_combo.setToolTip("Ctrl + D")


        self.sld_pen = QSlider(Qt.Horizontal, self)
        self.sld_pen.setMinimum(1)
        self.sld_pen.setMaximum(64)
        self.sld_pen.valueChanged.connect(self.change_pen_size)
        self.sld_pen.setToolTip("Change the brush size for the pen tool\nPress '-' to decrease the size, '=' to increase")

        self.sld_zoom = QSlider(Qt.Horizontal, self)
        self.zoom_values = [1/32,1/16,1/8,.25,.5,1,2,4,8,16]
        self.sld_zoom.setMinimum(1)
        self.sld_zoom.setMaximum(len(self.zoom_values))
        self.sld_zoom.valueChanged.connect(self.change_zoom)

        self.sld_opacity = QSlider(Qt.Horizontal, self)
        self.sld_opacity.setMinimum(1)
        self.sld_opacity.setMaximum(100)
        self.sld_opacity.valueChanged.connect(self.change_opacity)

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

        self.draw_terrain_chk = QCheckBox("Show Terrain",self)
        self.draw_terrain_chk.setChecked(True)
        self.draw_terrain_chk.setToolTip("Unselect this to hide terrain.")
        self.draw_terrain_chk.installEventFilter(self)

        self.align_grid_chk = QCheckBox("Align To Grid",self)
        self.align_grid_chk.setChecked(True)
        self.align_grid_chk.setToolTip("Select this if you wish to draw objects aligned to the grid.")
        self.align_grid_chk.installEventFilter(self)

        self.lbl_pen_size = QLabel('Pen Size: 1', self)

        self.lbl_zoom = QLabel('Zoom: ', self)
        self.lbl_opacity = QLabel('Object Opacity: ', self)

        self.lbl_map = QLabel('', self)

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

        self.editor.tile_set_name = "ground_set.png"
        # self.editor.tile_set_name = "ground_set2.png"
        self.editor.tile_set_path = self.root + ".."+slash+".."+slash+"img"+slash
        # self.editor.tile_set_path = self.root + "../../img/ground_set.png"
        self.editor.build_tiles()

        self.editor.object_set_path = self.root + ".."+slash+".."+slash+"img"+slash+"objects"+slash #TODO
        self.editor.build_objects()

        # directory
        self.map_dir = self.root+".."+slash

        # full path
        # self.map_path = self.map_dir+"test.map"
        self.map_path = ""

        self.auto_save_dir = self.root+"autosave"+slash

        if(not(os.path.isdir(self.map_dir))):
            os.makedirs(self.map_dir)

        if(not(os.path.isdir(self.auto_save_dir))):
            os.makedirs(self.auto_save_dir)


        self.grid = QGridLayout()
        self.grid.setSpacing(10)

        self.scroll_cs = 30
        self.scroll_rs = 30

        r = 0
        c = 0
        rs = self.scroll_rs
        cs = self.scroll_cs
        self.grid.addWidget(self.scroll, r, c, rs, cs)

        r += rs+1
        self.grid.addWidget(self.lbl_map, r, c, 1, cs)


        c = cs+c+1
        r = 0
        rs = 1
        cs = 2
        self.grid.addWidget(self.tool_combo, r, c, rs, cs)
        r += 2
        self.grid.addWidget(self.lbl_pen_size, r, c, rs, cs)
        r += 1
        self.grid.addWidget(self.sld_pen, r, c, rs, cs)

        r += 2
        self.grid.addWidget(self.lbl_zoom, r, c, rs, cs)
        r += 1
        self.grid.addWidget(self.sld_zoom, r, c, rs, cs)

        r += 2
        self.grid.addWidget(self.lbl_opacity, r, c, rs, cs)
        r += 1
        self.grid.addWidget(self.sld_opacity, r, c, rs, cs)


        r += 2
        self.grid.addWidget(self.draw_grid_chk, r, c, rs, 1)
        r += 1
        self.grid.addWidget(self.draw_terrain_chk, r, c, rs, 1)
        r += 1
        self.grid.addWidget(self.draw_objs_chk, r, c, rs, 1)
        r += 1
        self.grid.addWidget(self.align_grid_chk, r, c, rs, 1)
        r += 1
        self.grid.addWidget(self.draw_over_chk, r, c, rs, 1)

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

        self.sld_opacity.setValue(100)

        self.initialized = True

        self.repaint()

        self.save_timer = QTimer()
        # self.save_timer.timeout.connect(lambda x = self.appd + "temp_save.board":self.save_map(x))
        self.save_timer.timeout.connect(self.save_timer_cb)
        self.save_timer.start(1000*60)

        self.editor.right_click_cb = self.right_click_cb

    def right_click_cb(self, tile_index):
        if(tile_index is None): return
        if(tile_index == -1):
            item = self.tile_selector.item(0)
            item.setSelected(True)
            self.tile_selector_clicked(None)
        else:
            item = self.tile_selector.item(tile_index+1)
            item.setSelected(True)
            self.tile_selector_clicked(None)

    def incr_tile_selection(self, incr):
        selected = self.tile_selector.selectedItems()
        if(len(selected) == 0): return

        model_index = self.tile_selector.indexFromItem(selected[0])
        idx = model_index.row()

        new_idx = bound(idx+incr, 0, self.tile_selector.count())
        item = self.tile_selector.item(new_idx)
        item.setSelected(True)
        self.tile_selector_clicked(None)


    def save_timer_cb(self):
        self.auto_save("auto_save")

    def update_scroll_info(self):
        if(self.scroll is None): return
        # print(self.scroll.viewport().height())
        self.editor.view_height = self.scroll.viewport().height()
        self.editor.view_width = self.scroll.viewport().width()
        self.editor.view_c1_y = self.scroll.verticalScrollBar().value()
        self.editor.view_c4_y = self.editor.view_c1_y+self.editor.view_height
        self.editor.view_c1_x = self.scroll.horizontalScrollBar().value()
        self.editor.view_c4_x = self.editor.view_c1_x + self.editor.view_width

    def scrolled(self):
        self.repaint()

    def repaint(self):
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

        if(not(self.initialized)): return 0

        if(source is self.draw_grid_chk):
            self.editor.show_grid = self.draw_grid_chk.isChecked()
            self.repaint()

        elif(source is self.scroll):
            print("scroll")

        elif(source is self.draw_over_chk):
            self.editor.draw_over = self.draw_over_chk.isChecked()

        elif(source is self.draw_objs_chk):
            self.editor.show_objects = self.draw_objs_chk.isChecked()
            self.repaint()

        elif(source is self.draw_terrain_chk):
            self.editor.show_terrain = self.draw_terrain_chk.isChecked()
            self.repaint()

        elif(source is self.align_grid_chk):
            self.editor.align_objects = self.align_grid_chk.isChecked()


        if(event.type() == QEvent.KeyPress):
            key = event.key()
            modifiers = QApplication.keyboardModifiers()

            if(modifiers == Qt.ControlModifier):
                if(key == Qt.Key_S):
                    if(os.path.isfile(self.map_path)):
                        self.save_map(self.map_path)
                    else:
                        self.save_map("")

                elif(key == Qt.Key_O):
                    self.load_map("")

                elif(key == Qt.Key_T):
                    next_layer = self.editor.curr_layer+1
                    if(next_layer >= len(self.layer_list)):
                        next_layer = 0
                    self.layer_combo.setCurrentIndex(next_layer)
                    self.show_set_selector("")

                elif(key == Qt.Key_D):
                    lst = [x.lower() for x in self.tool_list]

                    next_index = lst.index(self.editor.tool)+1
                    if(next_index >= len(self.tool_list)):
                        next_index = 0

                    self.tool_combo.setCurrentIndex(next_index)
                    self.change_tool("")

                elif(key == Qt.Key_Z):
                    self.undo()

                elif(key == Qt.Key_C):
                    self.custom_close()


            elif(modifiers == Qt.NoModifier):
                # print(key)

                if(key == Qt.Key_Minus):
                    self.incr_pen_slider(-1)

                elif(key == Qt.Key_Equal):
                    self.incr_pen_slider(1)

                elif(key == Qt.Key_1):
                    self.incr_tile_selection(-1)
                elif(key == Qt.Key_2):
                    self.incr_tile_selection(1)

                elif(key == Qt.Key_Q):
                    self.custom_close()

                if(key == Qt.Key_T):
                    row = 0
                    col = 0
                    for i in range(len(self.editor.tile_set)):
                        self.editor.set_tile(row,col, i, True)
                        # if(self.tile_set[i].empty): continue
                        # self.editor.board[row][col].tile_index = i
                        col += 1
                        if(col >= 16):
                            row += 1
                            col = 0


        return 0

    def custom_close(self):
        self.auto_save("on_close")
        QCoreApplication.instance().quit()

    def closeEvent(self, event):
        self.custom_close()

    def change_tool(self,text):
        sel = self.tool_combo.currentText()
        self.editor.tool = sel.lower()

        if(self.editor.tool == "fill"):
            self.editor.change_list_disabled = True
        else:
            self.editor.change_list_disabled = False


    def incr_pen_slider(self, incr):
        value = self.sld_pen.value()
        new_value = bound(value+incr, self.sld_pen.minimum(), self.sld_pen.maximum())
        if(new_value == value): return
        self.sld_pen.setValue(new_value)

    def change_pen_size(self):
        self.editor.pen_size = max(1,int(self.sld_pen.value()))
        self.lbl_pen_size.setText("Pen Size: " + str(self.editor.pen_size))
        self.editor.mouseMoveEvent(None)

    def save_png(self):
        options = QFileDialog.Options()
        fileName, _ = QFileDialog.getSaveFileName(self,"Save",self.root,"PNG File (*.png);;All Files (*)", options=options)
        if(not fileName):
            return
        self.editor.png_path = fileName
        self.editor.save_png = True


    def undo(self):
        # print(self.editor.change_list)
        self.editor.editor_undo()
        self.editor.update()
        return

    def clear_map(self):
        self.auto_save("before_clear")
        self.editor.change_list_disabled = True
        for r in range(self.editor.board_height):
            for c in range(self.editor.board_width):
                self.editor.clear_tile(r, c)
        self.editor.change_list_disabled = False

    def num_to_uint8_bytes(self, num):
        return struct.pack("<B", num)

    def num_to_uint16_bytes(self, num):
        return struct.pack("<H", num)

    def num_to_uint32_bytes(self, num):
        return struct.pack("<L", num)

    def str_to_bytes(self, _str):
        return _str.encode()

    def uint8_bytes_to_num(self, _bytes):
        return struct.unpack("<B", bytes([_bytes]))[0]

    def uint16_bytes_to_num(self, _bytes):
        return struct.unpack("<H", bytes(_bytes))[0]

    def uint32_bytes_to_num(self, _bytes):
        return struct.unpack("<L", bytes(_bytes))[0]

    def bytes_to_str(self, _bytes):
        return _bytes.decode()

    def auto_save(self, fname):
        _add = ".map"
        if(fname.endswith(_add)):
            _add = ""
        self.save_map(self.auto_save_dir+fname+_add)


    def save_map(self, fpath=""):

        if(fpath == ""):

            _start = self.map_dir
            if(os.path.isfile(self.map_path)):
                _start = self.map_path

            options = QFileDialog.Options()
            fileName, _ = QFileDialog.getSaveFileName(self,"Save",_start,"Map Files (*.map);;All Files (*)", options=options)
            if(not fileName):
                return

            fpath = fileName

            self.map_path = fileName+""
            self.map_dir = slash.join(self.map_path.split(slash)[:-1]) + slash

            self.lbl_map.setText(self.map_path)


        printf("Saving map: %s\n", fpath)

        f = open(fpath, "wb")

        # id (1), data len (4), version (1), name len (1), name..., rows (2), cols (2), data...

        _id = 0
        _version = 0
        name_len = len(self.editor.tile_set_name)
        board_len = self.editor.board_height*self.editor.board_width
        total_len = 1+1+name_len+2+2+board_len

        f.write(self.num_to_uint8_bytes(_id))
        f.write(self.num_to_uint32_bytes(total_len))

        f.write(self.num_to_uint8_bytes(_version))

        f.write(self.num_to_uint8_bytes(name_len))
        f.write(self.str_to_bytes(self.editor.tile_set_name))

        f.write(self.num_to_uint16_bytes(self.editor.board_width))
        f.write(self.num_to_uint16_bytes(self.editor.board_height))

        for r in range(self.editor.board_height):
            for c in range(self.editor.board_width):
                ti = self.editor.board[r][c].tile_index
                if(ti < 0):
                    ti = 0xFF
                f.write(self.num_to_uint8_bytes(ti))

        f.close()

        printf("Saved map: %s\n", fpath)


    def load_map(self, fpath=""):

        self.auto_save("before_load")

        if(fpath == "" or not(os.path.isfile(fpath))):
            options = QFileDialog.Options()
            fileName, _ = QFileDialog.getOpenFileName(self,"Save",self.map_dir,"Map Files (*.map);;All Files (*)", options=options)
            if(not fileName):
                return

            self.map_path = fileName+""
            self.map_dir = slash.join(fileName.split(slash)[:-1]) + slash

            self.lbl_map.setText(self.map_path)

            fpath = fileName

        printf("Loading map: %s\n", fpath)

        f = open(fpath, "rb")
        b = f.read()
        f.close()

        idx = 0

        _id = self.uint8_bytes_to_num(b[idx])
        idx += 1

        total_len = self.uint32_bytes_to_num(b[idx:idx+4])
        idx += 4

        version = self.uint8_bytes_to_num(b[idx])
        idx += 1

        name_len = self.uint8_bytes_to_num(b[idx])
        idx += 1

        tile_set_name = self.bytes_to_str(b[idx:idx+name_len])
        idx += name_len

        brows = self.uint16_bytes_to_num(b[idx:idx+2])
        idx += 2

        bcols = self.uint16_bytes_to_num(b[idx:idx+2])
        idx += 2

        # data_len = len(b[idx:])
        data_len = total_len - (1 + 1 + name_len + 2 + 2)

        if(brows != self.editor.board_height):
            printf("[map load] ERROR: rows %d\n", brows)
            return

        if(bcols != self.editor.board_width):
            printf("[map load] ERROR: cols %d\n", bcols)
            return

        num_spaces = self.editor.board_height*self.editor.board_width
        if(data_len != num_spaces):
            printf("[map load] ERROR: data_len %d\n", data_len)
            return

        tpath = self.editor.tile_set_path + tile_set_name
        if(not(os.path.isfile(tpath))):
            printf("[map load] ERROR: file does not exist %s\n", tpath)
            return


        self.editor.change_list_disabled = True


        r = 0
        c = 0
        for i in range(num_spaces):
            ti = self.uint8_bytes_to_num(b[idx])
            if(ti == 0xFF): ti = -1
            idx += 1
            self.editor.set_tile_fast(r, c, ti)

            c += 1
            if(c >= self.editor.board_width):
                r += 1
                c = 0


        self.editor.change_list_disabled = False


        if(tile_set_name != self.editor.tile_set_name):
            self.editor.tile_set_name = tile_set_name
            self.editor.build_tiles()
            self.show_set_selector("")

        printf("Loaded map: %s\n", fpath)
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
                    _str = "Tile " + str(i)
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

        if(len(items) > 0):
            item = self.tile_selector.item(1)
            item.setSelected(True)
            self.editor.selected_index = 0


    def tile_selector_clicked(self,curr):

        if(curr is None):
            items = self.tile_selector.selectedItems()
            if(len(items) == 0): return
            itext = items[0].text()
        else:
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

        self.editor.update_zoom(zoom)
        self.repaint()

    def change_opacity(self):
        value = self.sld_opacity.value()
        self.editor.object_opacity = value/100
        self.lbl_opacity.setText("Object Opacity: " + str(value))
        self.repaint()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    w = MainWindow()
    w.show()
    # app.setQuitOnLastWindowClosed(False)
    app.exec_()

