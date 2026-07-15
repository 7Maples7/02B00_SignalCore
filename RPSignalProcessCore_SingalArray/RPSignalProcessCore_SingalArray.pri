QT       *= network

#QT       -= gui

#TARGET = cudaQTS
#CONFIG   += console
#CONFIG   -= app_bundle

#TEMPLATE = app


#if(contains(DEFINES,Mac_Radar_Archiwave)){
#HEADERS+= \
#  $$PWD/include/archive/CDefine.h \
#  INCLUDEPATH+= $$PWD/include/archive/

#}else{
#HEADERS+= \
#$$PWD/include/rdv/CDefine.h \
#INCLUDEPATH+= $$PWD/include/rdv/ \
#}

INCLUDEPATH+= $$PWD/

INCLUDEPATH+= $$PWD/include

HEADERS += \
    $$PWD/CSignalProcessThreadMng.h \
    $$PWD/include/GPrintDef.h \
    $$PWD/include/blas.h \
    $$PWD/include/common.h \
    $$PWD/include/RCSignalProcessMain.h \
    $$PWD/include/init.h \
    $$PWD/include/kernel.h


SOURCES +=  \
    $$PWD/CSignalProcessThreadMng.cpp \
    $$PWD/src/GPrintDef.cpp \
    $$PWD/src/RCSignalProcessMain.cpp \
    $$PWD/src/init.cpp \

# This makes the .cu files appear in your project
OTHER_FILES +=  $$PWD/src/kernel.cu\
                $$PWD/src/blas.cu



# CUDA settings <-- may change depending on your system
CUDA_SOURCES += $$PWD/src/kernel.cu\
                $$PWD/src/blas.cu\

CUDA_SDK = "/usr/local/cuda-13.0/"   # Path to cuda SDK install
CUDA_DIR = "/usr/local/cuda-13.0/"            # Path to cuda toolkit install

# DO NOT EDIT BEYOND THIS UNLESS YOU KNOW WHAT YOU ARE DOING....

SYSTEM_NAME = ubuntu         # Depending on your system either 'Win32', 'x64', or 'Win64'
SYSTEM_TYPE = 64            # '32' or '64', depending on your system
CUDA_ARCH = sm_120           # Type of CUDA architecture, for example 'compute_10', 'compute_11', 'sm_10'
NVCC_OPTIONS = --use_fast_math


#sudo cp /usr/local/cuda-10.0/lib64/libcudart.so.10.0 /usr/local/lib/libcudart.so.10.0 && sudo ldconfig
#sudo cp /usr/local/cuda-10.0/lib64/libcublas.so.10.0 /usr/local/lib/libcublas.so.10.0 && sudo ldconfig
#sudo cp /usr/local/cuda-10.0/lib64/libcurand.so.10.0 /usr/local/lib/libcurand.so.10.0 && sudo ldconfig
#sudo cp /usr/local/cuda-10.0/lib64/libnppidei.so.10.0 /usr/local/lib/libnppidei.so.10.0 && sudo ldconfig
#sudo cp /usr/local/cuda-10.0/lib64/libnppidei.so.10.0 /usr/local/lib/libnppidei.so.10.0 && sudo ldconfig
#sudo cp /usr/local/cuda-10.0/lib64/libnppc.so.10.0 /usr/local/lib/libnppc.so.10.0 && sudo ldconfig


# include paths
CUDA_VERSION = 13.0

INCLUDEPATH += $$CUDA_DIR/include\
               $$PWD/include/\
               $$PWD/\
               /usr/local/cuda-13.0\
               /usr/local/cuda-13.0/include\



# library directories
QMAKE_LIBDIR += $$CUDA_DIR/lib64/

CUDA_OBJECTS_DIR = ./


# Add the necessary libraries
CUDA_LIBS = -lcuda -lcudart -lcufft -lnppidei -lnppc -lnppicc -lcublas

# The following makes sure all path names (which often include spaces) are put between quotation marks
CUDA_INC = $$join(INCLUDEPATH,'" -I"','-I"','"')
#LIBS += $$join(CUDA_LIBS,'.so ', '', '.so')

LIBS += $$CUDA_LIBS

# Configuration of the Cuda compiler
CONFIG(debug, debug|release) {
    # Debug mode
    cuda_d.input = CUDA_SOURCES
    cuda_d.output = $$CUDA_OBJECTS_DIR/${QMAKE_FILE_BASE}_cuda.o
    cuda_d.commands = $$CUDA_DIR/bin/nvcc --default-stream per-thread -D_DEBUG $$NVCC_OPTIONS $$CUDA_INC $$NVCC_LIBS --machine $$SYSTEM_TYPE -arch=$$CUDA_ARCH -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    cuda_d.dependency_type = TYPE_C
    QMAKE_EXTRA_COMPILERS += cuda_d
}
else {
    # Release mode
    cuda.input = CUDA_SOURCES
    cuda.output = $$CUDA_OBJECTS_DIR/${QMAKE_FILE_BASE}_cuda.o
    cuda.commands = $$CUDA_DIR/bin/nvcc --default-stream per-thread $$NVCC_OPTIONS $$CUDA_INC $$NVCC_LIBS --machine $$SYSTEM_TYPE -arch=$$CUDA_ARCH -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    cuda.dependency_type = TYPE_C
    QMAKE_EXTRA_COMPILERS += cuda
}

DISTFILES += \
    $$PWD/src/blas.cu \
    $$PWD/src/kernal.cu

RESOURCES += \
#    $$PWD/coel/coel.qrc

