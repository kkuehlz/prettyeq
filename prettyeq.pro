TEMPLATE = subdirs
CONFIG +=  ordered
SUBDIRS = \
          equalizer \
          gui

gui.depends = equalizer
