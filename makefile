

COMPILER = clang
SOURCE_LIBS = -Ilib/ -I/opt/homebrew/Cellar/ffmpeg/7.0.1/include
OSX_OPT = -Llib/ -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL lib/libraylib.a -L/opt/homebrew/Cellar/ffmpeg/7.0.1/lib
OSX_OUT = -o "bin/build_osx"
CFILES = src/*.c

FFMPEG_LIBS = -lavformat -lavcodec -lavutil -lswscale -lswresample


build_osx: 
		$(COMPILER) $(CFILES) $(SOURCE_LIBS) $(OSX_OUT) $(OSX_OPT) $(FFMPEG_LIBS)
