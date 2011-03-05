#-------------------------------------------------
#
# Project created by QtCreator 2010-10-22T22:07:57
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = TheGame
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

LIBS += -lsfml-graphics -lsfml-window -lsfml-system -lBox2D

SOURCES += main.cpp \
    World.cpp \
    TMatrix.cpp \
    Sprite.cpp \
    Rope.cpp \
    Point.cpp \
    Player.cpp \
    DummyDrawable.cpp \
    ContactListener.cpp \
    Block.cpp \
    Bridge.cpp

OTHER_FILES += \
    TheGame.pro.user \
    data/smb3_mario_sheet.png \
    data/rope.tga \
    data/mario_walk_2_right.tga \
    data/mario_walk_2_left.tga \
    data/mario_walk_1_right.tga \
    data/mario_walk_1_left.tga \
    data/mario_stand_right.tga \
    data/mario_stand_left.tga \
    data/mariosmall.tga \
    data/mario_jump_right.tga \
    data/mario_jump_left.tga \
    data/mario_break_right.tga \
    data/mario_break_left.tga \
    data/mario.tga \
    data/level.txt \
    data/groundtile.tga \
    data/apa_walk_right_2.tga \
    data/apa_walk_right_2.png \
    data/apa_walk_right_1.xcf \
    data/apa_walk_right_1.tga \
    data/apa_walk_right_1.png

HEADERS += \
    World.h \
    TMatrix.h \
    Sprite.h \
    Rope.h \
    Point.h \
    Player.h \
    IRotateable.h \
    IPositionable.h \
    IDrawable.h \
    DummyDrawable.h \
    ContactListener.h \
    Block.h \
    Bridge.h
