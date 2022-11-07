import sys, os, math

from PyQt5.QtGui import QPainter, QImage
from PyQt5.QtCore import Qt, QSize


slash = "/"
if sys.platform == "win32":
    slash = "\\"

root = os.path.dirname(os.path.abspath(__file__)) + slash + "blender_output" + slash

def bound(_val, _min, _max):
    return max(min(_val, _max), _min)

def printf(fmt, *args):
    if(len(args) > 0):
        print(fmt % args, end="")
    else:
        print(fmt, end="")


def concat_images(img_lst, cols):
    largest_w = img_lst[0].width()
    largest_h = img_lst[0].height()

    rng = range(len(img_lst))

    for i in rng:
        img = img_lst[i]
        if(img.width() > largest_w):
            largest_w = img.width()
        if(img.height() > largest_h):
            largest_h = img.height()

    rows = math.ceil(len(img_lst)/cols)
    height = largest_h * rows
    width = largest_w * cols

    image = QImage(width, height, QImage.Format_ARGB32)

    painter = QPainter()
    painter.begin(image)

    row = 0
    col = 0
    for i in rng:
        x = col*largest_w
        y = row*largest_h
        painter.drawImage(x,y,img_lst[i])
        col += 1
        if(col >= cols):
            row += 1
            col = 0

    painter.end()
    return image



class Compositor():
    def __init__(self, model, model_set, scale_w, scale_h, save=False):

        self.composite_image = None

        self.slash = "/"
        if sys.platform == "win32":
            self.slash = "\\"
        # self.root = os.path.dirname(os.path.abspath(__file__)) + self.slash + "blender_output" + self.slash
        self.root = root

        self.model = model
        self.model_set = model_set

        self.path = self.root + self.model + self.slash + self.model_set + self.slash

        if(not(self.path.endswith(slash))):
            self.path += slash

        if(not(os.path.isdir(self.path))):
            printf("Directory doesn't exist: %s\n", self.path)
            return


        # self.folders = ["walk_normal", "walk_gun_ready"]
        self.orientations = ["front" , "front_right", "right", "back_right", "back", "back_left", "left", "front_left"]

        self.scaled_size = QSize(scale_w, scale_h)
        self.num_rows = len(self.orientations)
        self.num_cols = 0

        nums = ["00%02d" % i for i in range(100)]

        self.pic_data = {}
        for j in range(len(self.orientations)):
            o = self.orientations[j]

            search_pics = [n + "_" + o + ".png" for n in nums]

            pics = [x.lower() for x in os.listdir(self.path) if os.path.isfile(self.path+x) and x.lower() in search_pics]
            pics.sort()

            self.num_cols = len(pics)

            for p in pics:
                self.pic_data[self.path+p] = {}

        self.keys = [x for x in self.pic_data.keys()]
        if(len(self.keys) == 0):
            printf("No images found\n")
            return

        printf("%s\n", self.path)
        self.krange = range(len(self.keys))
        for i in self.krange:
            path = self.keys[i]
            printf("Loading image %d of %d      \r", i+1, len(self.keys))
            img = QImage(path,"PNG")
            img = img.convertToFormat(QImage.Format_ARGB32)
            # https://doc.qt.io/qtforpython-5/PySide2/QtGui/QImage.html?highlight=qimage#PySide2.QtGui.PySide2.QtGui.QImage.scaled
            # https://doc.qt.io/qtforpython-5/PySide2/QtCore/Qt.html#PySide2.QtCore.PySide2.QtCore.Qt.AspectRatioMode
            img_scaled = img.scaled(self.scaled_size, Qt.IgnoreAspectRatio, Qt.SmoothTransformation)
            self.pic_data[path]["img"] = img
            self.pic_data[path]["img_scaled"] = img_scaled

        print("")

        self.composite_image = self.draw_png()

        if(save):
            self.save_png()

    def draw_png(self):

        tw = self.scaled_size.width()*self.num_cols
        th = self.scaled_size.height()*self.num_rows

        image = QImage(tw, th, QImage.Format_ARGB32)

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

        painter.end()
        return image

    def save_png(self):
        if(self.composite_image is None):
            printf("Composite image is None\n")
            return

        path = self.root + self.model + "-" + self.model_set + ".png"
        self.composite_image.save(path, "PNG")
        printf("Saved: %s\n", path)


def main():

    PROCESS_ALL = True
    PROCESS_ALL = False

    # root = os.path.dirname(os.path.abspath(__file__)) + slash + "blender_output" + slash

    if(PROCESS_ALL):
        models = [x for x in os.listdir(root) if os.path.isdir(root+x) and not(x.startswith("!"))]

        for m in models:
            mpath = root + m + slash
            model_sets = [x for x in os.listdir(mpath) if os.path.isdir(mpath+x) and not(x.startswith("!"))]
            if(len(model_sets) == 0):
                continue
            for ms in model_sets:
                c = Compositor(m,ms, 128, 128, True)
                print("")

    else:

        c = Compositor("human1_0","attack1", 128, 128, True)

        # @TEST
        # # img = QImage(32, 32, QImage.Format_ARGB32)
        # img_concat = concat_images([c.composite_image]*3, 2)
        # img_concat.save(root + "test.png", "PNG")




if __name__ == "__main__":
    main()

